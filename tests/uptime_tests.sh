#!/bin/bash -x
send value_request 9dof.uptime; sleep 3
send value_request arm.uptime; sleep 3
send value_request cam.uptime; sleep 3
send value_request diag.uptime; sleep 3
send value_request drive.uptime; sleep 3
send value_request enviro.uptime; sleep 3
send value_request modema.uptime; sleep 3
send sos_nostfu modemb; sleep 3
send value_request modemb.uptime
