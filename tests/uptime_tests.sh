#!/bin/bash -x
send value_request 9dof.uptime; sleep 5
send value_request arm.uptime; sleep 5
send value_request cam.uptime; sleep 5
send value_request diag.uptime; sleep 5
send value_request drive.uptime; sleep 5
send value_request enviro.uptime; sleep 5
send value_request modema.uptime; sleep 5
send sos_nostfu modemb; sleep 10
send value_request modemb.uptime; sleep 5
send sos_stfu modemb; sleep 5
