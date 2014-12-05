/*
 * userspace/gpsd/gpsd.c
 * User space daemon polling GPS location data
 *
 * Copyright (C) 2014 V. Atlidakis, G. Koloventzos, A. Papancea
 *
 * COMS W4118 Fall 2014, Columbia University
 *
 * Last updated: 11/26/2014
 */
#include "gpsd.h"
#include <time.h>

#define LOGFILE "/data/misc/gpstrace.log"
#define DBG(fmt, ...) fprintf(fp, fmt, ## __VA_ARGS__)
static FILE *fp;

/*
 * Turn calling process into a daemon
 */
void daemonize(void)
{
	pid_t pid;

	pid = fork();
	if (pid < 0) {
		perror("fork");
		exit(EXIT_FAILURE);
	}

	if (pid > 0)
		exit(EXIT_SUCCESS);

	if (setsid() < 0) {
		perror("setsid");
		exit(EXIT_FAILURE);
	}
	close(0);
	close(1);
	close(2);
	chdir("/data/misc/");
	umask(0);
}

void poll_gps_data(void)
{
	FILE *file;
	int rval;
	struct gps_location *loc;

	loc = malloc(sizeof(struct gps_location));
	if (loc == NULL) {
		perror("malloc");
		goto exit;
	}

	file = fopen(GPS_LOCATION_FILE, "r");
	if (file == NULL) {
		perror("fopen");
		goto free;
	}

	if (fscanf(file, "%lf %lf %f",
		   &loc->latitude, &loc->longitude, &loc->accuracy) != 3) {
		perror("fscanf");
		goto close;
	}

	DBG("%u - lat: %f, lng: %f, accuracy: %f\n", (unsigned)time(NULL),
						     loc->latitude,
						     loc->longitude,
						     loc->accuracy);

	rval = set_gps_location(loc);
	if (rval != 0) {
		perror("set_gps_location");
		DBG("failed to write gps data to kernel\n");
	}

close:	fclose(file);
free:	free(loc);
exit:	return;
}

int main(int argc, char *argv[])
{
	/* turn me into daemon */
	daemonize();

	printf("daemon: start polling for gps data\n");

	fp = fopen(LOGFILE, "w+");
	if (fp == NULL) {
		perror("fopen");
		exit(EXIT_FAILURE);
	}

	while (1) {
		poll_gps_data();
		usleep(200000);
	}

	fclose(fp);

	return 0;
}
