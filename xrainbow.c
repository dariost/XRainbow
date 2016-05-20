/*
Licensed to the Apache Software Foundation (ASF) under one
or more contributor license agreements.  See the NOTICE file
distributed with this work for additional information
regarding copyright ownership.  The ASF licenses this file
to you under the Apache License, Version 2.0 (the
"License"); you may not use this file except in compliance
with the License.  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
KIND, either express or implied.  See the License for the
specific language governing permissions and limitations
under the License.
*/

#include <X11/Xlib.h>
#include <X11/extensions/xf86vmode.h>
#include <assert.h>
#include <errno.h>
#include <math.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define XRAINBOW_MAJOR 1
#define XRAINBOW_MINOR 0
#define XRAINBOW_PATCH 1

Display* x11_display = NULL;
int x11_screen = 0;
int pending_quit = 0;

double get_time()
{
    struct timespec t;
    errno = 0;
    if(clock_gettime(CLOCK_MONOTONIC, &t))
    {
        fprintf(stderr, "Error while calling clock_gettime: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    return ((double)t.tv_sec) + ((double)t.tv_nsec / 1.0e9);
}

void change_gamma(float r, float g, float b)
{
    assert(r >= 0.1f && r <= 10.0f);
    assert(g >= 0.1f && g <= 10.0f);
    assert(b >= 0.1f && b <= 10.0f);
    XF86VidModeGamma color = {r, g, b};
    if(!XF86VidModeSetGamma(x11_display, x11_screen, &color))
    {
        fprintf(stderr, "Error while calling XF86VidModeSetGamma\n");
        exit(EXIT_FAILURE);
    }
    XFlush(x11_display);
}

void quit_application()
{
    change_gamma(1.0f, 1.0f, 1.0f);
    if(x11_display)
    {
        XCloseDisplay(x11_display);
        x11_display = NULL;
    }
}

void handle_signal(int param)
{
    (void)param;
    pending_quit = 1;
}

void print_version_and_exit()
{
    printf("XRainbow version %d.%d.%d\n", XRAINBOW_MAJOR, XRAINBOW_MINOR, XRAINBOW_PATCH);
    printf("\n");
    printf("Copyright 2016 | Dario Ostuni <another.code.996@gmail.com>\n");
    printf("\n");
    printf("Licensed to the Apache Software Foundation (ASF) under one\n");
    printf("or more contributor license agreements.  See the NOTICE file\n");
    printf("distributed with this work for additional information\n");
    printf("regarding copyright ownership.  The ASF licenses this file\n");
    printf("to you under the Apache License, Version 2.0 (the\n");
    printf("\"License\"); you may not use this file except in compliance\n");
    printf("with the License.  You may obtain a copy of the License at\n");
    printf("\n");
    printf("    http://www.apache.org/licenses/LICENSE-2.0\n");
    printf("\n");
    printf("Unless required by applicable law or agreed to in writing,\n");
    printf("software distributed under the License is distributed on an\n");
    printf("\"AS IS\" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY\n");
    printf("KIND, either express or implied.  See the License for the\n");
    printf("specific language governing permissions and limitations\n");
    printf("under the License.\n");
    exit(EXIT_SUCCESS);
}

void print_usage_and_exit(char* program, int return_code)
{
    printf("Usage: %s [OPTIONS]\n\n", program);
    printf("\t-h | --help\t\tPrints this help\n");
    printf("\t-v | --version\t\tPrints version and copyright info\n");
    printf("\t-t | --time-limit\tTime limit (float) in seconds, -1 for infinite\n");
    printf("\t-s | --speed\t\tRainbow speed (float), range (0; INFINITY)\n");
    printf("\t-l | --luminosity\tBase luminosity (float), range [0.1; 9.9]\n");
    exit(return_code);
}

void call_rainbow(double t, float c)
{
    float palette[3] = {c, c, c};
    palette[((long)t) % 3] += 1.0f - fmod(t, 1.0);
    palette[((long)t + 1) % 3] += fmod(t, 1.0);
    change_gamma(palette[0], palette[1], palette[2]);
}

int main(int argc, char* argv[])
{
    double time_limit = -1.0;
    double speed = 1.0;
    float luminosity = 1.0f / 3.0f;
    for(int i = 1; i < argc; i++)
    {
        if(!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help"))
        {
            print_usage_and_exit(argv[0], EXIT_SUCCESS);
        }
        else if(!strcmp(argv[i], "-v") || !strcmp(argv[i], "--version"))
        {
            print_version_and_exit();
        }
        else if((!strcmp(argv[i], "-t") || !strcmp(argv[i], "--time-limit")) && i + 1 < argc)
        {
            time_limit = atof(argv[i + 1]);
            i++;
        }
        else if((!strcmp(argv[i], "-l") || !strcmp(argv[i], "--luminosity")) && i + 1 < argc)
        {
            luminosity = atof(argv[i + 1]);
            if(luminosity < 0.1f || luminosity > 9.9f)
            {
                print_usage_and_exit(argv[0], EXIT_FAILURE);
            }
            i++;
        }
        else if((!strcmp(argv[i], "-s") || !strcmp(argv[i], "--speed")) && i + 1 < argc)
        {
            speed = atof(argv[i + 1]);
            if(speed <= 0.0f)
            {
                print_usage_and_exit(argv[0], EXIT_FAILURE);
            }
            i++;
        }
        else
        {
            print_usage_and_exit(argv[0], EXIT_FAILURE);
        }
    }
    errno = 0;
    x11_display = XOpenDisplay(NULL);
    if(!x11_display)
    {
        fprintf(stderr, "Error while calling XOpenDisplay: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }
    x11_screen = DefaultScreen(x11_display);
    atexit(quit_application);
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    signal(SIGSEGV, handle_signal);
    double start_time = get_time();
    while(time_limit < 0.0 || time_limit >= get_time() - start_time)
    {
        call_rainbow((get_time() - start_time) * speed, luminosity);
        sched_yield();
        if(pending_quit)
            break;
    }
    return EXIT_SUCCESS;
}
