#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <argp.h>

#include "tcp_analyzer.h"

const char *argp_program_version = "tcp_analyse v1.0";
const char *argp_program_bug_address = "jm.tcp_analyse@gmail.com";
static char doc[] = "filters out TCP traffic with user-defined rules";
static char args_doc[] = "[FILENAME]...";


static struct argp_option options[] = { 
    { "outfile", 'f', "OUTFILE", 0, "Output file" },
    { "interface", 'i', "INTERFACE", 0, "Interface name" },
    { "dump", 0x100, 0 , 0, "Dump data" },
    { "promiscuous", 0x200, 0, 0, "Turns on promiscuous mode" },
    { "max_length", 0x300, "MAX_LENGTH", 0, "Maximum packet length" },
    { "min_length", 0x400, "MIN_LENGTH", 0, "Minimum packet length" },
    { "packets", 'p', "PACKET_NUMBER" , 0, "Packets to receive" },
    { "src_port", 's', "SOURCE_PORT", 0, "Filter on source port" },
    { "dst_port", 'd', "DESTINATION_PORT", 0, "Filter on destination port" },
    { "ip_source", 'm', "IP_SRC", 0, "Filter on source ip address" },
    { "ip_dest", 'r', "IP_DST", 0, "Filter on destination ip address" },
    { 0 }
};


static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    struct arguments *arguments = state->input;
    // printf("KEY %s", key);
    switch (key) {
    case 0x100: arguments->data_dump = true; break;
    case 0x200: arguments->promiscuous = true; break;
    case 0x300: arguments->max_length = (int) strtol(arg, NULL, 10); break;
    case 0x400: arguments->min_length = (int) strtol(arg, NULL, 10); break;
    case 'f': arguments->out_file = arg; break;
    case 'i': arguments->interface = arg; break;
    case 'p': arguments->packets = (int) strtol(arg, NULL, 10); break;
    case 's': arguments->src_port = (int) strtol(arg, NULL, 10); break;
    case 'd': arguments->dst_port = (int) strtol(arg, NULL, 10); break;
    case 'm': arguments->ip_source = arg; break;
    case 'r': arguments->ip_dest = arg; break;
    case ARGP_KEY_ARG: return 0;
    default: return ARGP_ERR_UNKNOWN;
    }   
    return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };

int main(int argc, char **argv)
{
    struct arguments arguments;
    // default options
    arguments.interface = "";
    arguments.data_dump = false;
    arguments.promiscuous = false;
    arguments.packets = 20;
    arguments.src_port  = 0;
    arguments.dst_port  = 0;
    arguments.max_length = 20480;
    arguments.min_length = 0;
    arguments.ip_source = "";
    arguments.ip_dest = "";
    arguments.out_file = "";

    argp_parse(&argp, argc, argv, 0, 0, &arguments);
    printf("ARGUMENTS\n");
    printf("INTERFACE : %s\n", arguments.interface);
    printf("PACKETS %d\n", arguments.packets);
    printf("SRC PORT : %d\n", arguments.src_port);
    printf("DST PORT : %d\n", arguments.dst_port);
    printf("SRC IP : %s\n", arguments.ip_source);
    printf("DST IP : %s\n", arguments.ip_dest);
    run_analyser(&arguments);
    printf("FINISHED\n");
    return 0;
}
