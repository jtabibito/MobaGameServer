#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "service.h"

// return false, close socket
// else continue execute 
bool
service::on_session_recv_cmd(session* s,
	struct cmd_msg* msg) {
	return false;
}

// if lose connection
void
service::on_session_disconnect(session* s) {

}