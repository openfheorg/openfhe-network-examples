/***
 * � 2021 Duality Technologies, Inc. All rights reserved.
 * This is a proprietary software product of Duality Technologies, Inc.
 *protected under copyright laws and international copyright treaties, patent
 *law, trade secret law and other intellectual property rights of general
 *applicability. Any use of this software is strictly prohibited absent a
 *written agreement executed by Duality Technologies, Inc., which provides
 *certain limited rights to use this software. You may not copy, distribute,
 *make publicly available, publicly perform, disassemble, de-compile or reverse
 *engineer any part of this software, breach its security, or circumvent,
 *manipulate, impair or disrupt its operation.
 ***/
#include "thresh_server.h"

int main(int argc, char** argv) {
	Params params;
	//char optstring[] = "i:p:l:n:W:h";
	if (!processInputParams(argc, argv, params)) {
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}
	ServerImpl server;
	server.Run(params);

	return 0;
}
