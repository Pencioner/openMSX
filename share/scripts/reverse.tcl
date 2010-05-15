namespace eval reverse {

	# Enable reverse if not yet enabled. As an optimization, also return
	# reverse-status info (so that caller doesn't have to query it again).
	proc auto_enable {} {
		set stat_list [reverse status]
		array set stat $stat_list
		if {$stat(status) == "disabled"} {
			reverse start
		}
		return $stat_list
	}

	set_help_text reverse_prev \
{Go back in time to the previous 'snapshot'.
A 'snapshot' is actually an internal implementation detail, but the\
important thing for this command is that the further back in the past,\
the less dense the snapshots are. So executing this command multiple times\
will take successively bigger steps in the past. Going back to a snapshot\
is also slightly faster than going back to an arbitrary point in time\
(let's say going back a fixed amount of time).
}
	proc reverse_prev {{minimum 1} {maximum 15}} {
		array set revstat [auto_enable]
		if {[llength $revstat(snapshots)] == 0} return

		set upperTarget [expr $revstat(current) - $minimum]
		set lowerTarget [expr $revstat(current) - $maximum]

		# search latest snapshot that is still before upperTarget
		set i [expr [llength $revstat(snapshots)] - 1]
		while {([lindex $revstat(snapshots) $i] > $upperTarget) && ($i > 0)} {
			incr i -1
		}
		# but don't go below lowerTarget
		set t [lindex $revstat(snapshots) $i]
		if {$t < $lowerTarget} {
			set t $lowerTarget
		}

		reverse goto $t
	}

	set_help_text reverse_next \
{This is very much like 'reverse_prev', but instead it goes to the closest\
snapshot in the future (if possible).
}
	proc reverse_next {{minimum 0} {maximum 15}} {
		array set revstat [auto_enable]
		if {[llength $revstat(snapshots)] == 0} return

		set lowerTarget [expr $revstat(current) + $minimum]
		set upperTarget [expr $revstat(current) + $maximum]

		# search first snapshot that is after lowerTarget
		lappend revstat(snapshots) $revstat(end)
		set l [llength $revstat(snapshots)]
		set i 0
		while {($i < $l) && ([lindex $revstat(snapshots) $i] < $lowerTarget)} {
			incr i
		}

		if {$i < $l} {
			# but don't go above upperTarget
			set t [lindex $revstat(snapshots) $i]
			if {$t > $upperTarget} {
				set t $upperTarget
			}
			reverse goto $t
		}
	}

	# note: you can't use bindings with modifiers like SHIFT, because they
	# will already stop the replay, as they are MSX keys as well
	bind_default PAGEUP "reverse_prev"
	bind_default PAGEDOWN "reverse_next"

	proc after_switch {} {
		if {$::auto_enable_reverse == "on"} {
			auto_enable
		} elseif {$::auto_enable_reverse == "gui"} {
			reverse_widgets::enable_reversebar false
		}
		after machine_switch [namespace code after_switch]
	}


	namespace export reverse_prev
	namespace export reverse_next
}

namespace eval reverse_widgets {
	variable update_after_id
	variable mouse_after_id

	set_help_text toggle_reversebar \
{Enable/disable an on-screen reverse bar.
This will show the recorded 'reverse history' and the current position in\
this history. It is possible to click on this bar to immediately jump to a\
specific point in time. If the current position is at the end of the history\
(i.e. we're not replaying an existing history), this reverse bar will slowly\
fade out. You can make it reappear by moving the mouse over it.
}
	proc toggle_reversebar {} {
		if {[catch {osd info reverse}]} {
			enable_reversebar
		} else {
			disable_reversebar
		}
		return ""
	}

	proc enable_reversebar {{visible true}} {
		reverse::auto_enable

		if {![catch {osd info reverse}]} {
			# osd already enabled
			return
		}

		# Hack: put the reverse bar at the top/bottom when the icons
		#  are at the bottom/top. It would be better to have a proper
		#  layout manager for the OSD stuff.
		if {[catch {set led_y [osd info osd_icons -y]}]} {
			set led_y 0
		}
		set y [expr ($led_y == 0) ? 232 : 2]

		set fade [expr $visible ? 1.0 : 0.0]
		osd create rectangle reverse \
			-scaled true -x 35 -y $y -w 250 -h 6 \
			-rgba 0x00000080 -fadeCurrent $fade -fadeTarget $fade
		osd create rectangle reverse.top \
			-x -1 -y   -1 -relw 1 -w 2 -h 1 -z 4 -rgba 0xFFFFFFC0
		osd create rectangle reverse.bottom \
			-x -1 -rely 1 -relw 1 -w 2 -h 1 -z 4 -rgba 0xFFFFFFC0
		osd create rectangle reverse.left \
			-x -1         -w 1 -relh 1      -z 4 -rgba 0xFFFFFFC0
		osd create rectangle reverse.right \
			-relx 1       -w 1 -relh 1      -z 4 -rgba 0xFFFFFFC0
		osd create rectangle reverse.bar \
			-relw 0 -relh 1                 -z 0 -rgba 0x0077FF80
		osd create rectangle reverse.end \
			-relx 0 -x -1 -w 2 -relh 1      -z 2 -rgba 0xFF8080C0
		osd create text      reverse.text \
			-x -10 -y 0 -relx 0.5 -size 5   -z 3 -rgba 0xFFFFFFC0

		update_reversebar

		variable mouse_after_id
		set mouse_after_id [after "mouse button1 down" [namespace code check_mouse]]
	}

	proc disable_reversebar {} {
		variable update_after_id
		variable mouse_after_id
		catch { after cancel $update_after_id }
		catch { after cancel $mouse_after_id  }
		catch { osd destroy reverse }
	}

	proc update_reversebar {} {
		array set stats [reverse status]

		switch $stats(status) {
		"disabled" {
			disable_reversebar
			return
		}
		"replaying" {
			osd configure reverse -fadeTarget 1.0 -fadeCurrent 1.0
		}
		"enabled" {
			set x 2 ; set y 2
			catch { foreach {x y} [osd info "reverse" -mousecoord] {} }
			if {0 <= $x && $x <= 1 && 0 <= $y && $y <= 1} {
				osd configure reverse -fadePeriod 0.5 -fadeTarget 1.0
			} else {
				osd configure reverse -fadePeriod 5.0 -fadeTarget 0.0
			}
		}}

		set snapshots $stats(snapshots)
		set totLenght [expr $stats(end) - $stats(begin)]
		set playLength [expr $stats(current) - $stats(begin)]
		set reciprocalLength [expr ($totLenght != 0) ? (1.0 / $totLenght) : 0]
		set fraction [expr $playLength * $reciprocalLength]

		set count 0
		foreach snapshot $snapshots {
			set name reverse.tick$count
			catch {
				# create new if it doesn't exist yet
				osd create rectangle $name -w 0.5 -relh 1 -rgb 0x444444 -z 1
			}
			osd configure $name -relx [expr ($snapshot - $stats(begin)) * $reciprocalLength]
			incr count
		}
		while true {
			# destroy all with higher count number
			if {[catch {osd destroy reverse.tick$count}]} {
				break
			}
			incr count
		}

		osd configure reverse.bar -relw $fraction
		osd configure reverse.end -relx $fraction
		osd configure reverse.text \
			-text "[formatTime $playLength] / [formatTime $totLenght]"
		variable update_after_id
		set update_after_id [after realtime 0.10 [namespace code update_reversebar]]
	}

	proc check_mouse {} {
		set x 2 ; set y 2
		catch { foreach {x y} [osd info "reverse" -mousecoord] {} }
		if {0 <= $x && $x <= 1 && 0 <= $y && $y <= 1} {
			array set stats [reverse status]
			reverse goto [expr $stats(begin) + $x * ($stats(end) - $stats(begin))]
		}
		variable mouse_after_id
		set mouse_after_id [after "mouse button1 down" [namespace code check_mouse]]
	}

	proc formatTime {seconds} {
		format "%02d:%02d" [expr int($seconds / 60)] [expr int($seconds) % 60]
	}

	namespace export toggle_reversebar
}

namespace import reverse::*
namespace import reverse_widgets::*

user_setting create string "auto_enable_reverse" \
{Controls whether the reverse feature is automatically enabled on startup.
Internally the reverse feature takes regular snapshots of the MSX state,
this has a (small) cost in memory and in performance. On small systems you
don't want this cost, so we don't enable the reverse feature by default.
Possible values for this setting:
  off   Reverse not enabled on startup
  on    Reverse enabled on startup
  gui   Reverse + reverse_bar enabled (see 'help toggle_reversebar')
} off


# TODO hack:
# The order in which the startup scripts are executed is not defined. But this
# needs to run after the load_icons script has been executed.
#
# It also needs to run after 'after boot' proc in the autoplug.tcl script. The
# reason is tricky:
#  - If this runs before autoplug, then the initial snapshot (initial snapshot
#    triggered by enabling reverse) does not contain the plugged cassette player
#    yet.
#  - Shortly after the autoplug proc runs, this will plug the cassetteplayer.
#    This plug command will be recorded in the event log. This is fine.
#  - The problem is that we don't serialize information for unplugged devices
#    ((only) in the initial snapshot, the cassetteplayer is not yet plugged). So
#    the information about the inserted tape is lost.
# As a temporary solution/hack, we call 'reverse::after_switch' from the
# autoplug script (so we're sure it runs after the auto plugging). An
# alternative is to also serialize the state of unplugged pluggables.
#
#after boot {
#	reverse::after_switch
#}