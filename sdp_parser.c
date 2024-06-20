#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define SENDER_RTCP_BW_PERCENT 0.0125  // 1.25% Based on rfc3556 manual, page 3
#define RECEIVER_RTCP_BW_PERCENT 0.0375 // 3.75% Based on rfc3556 manual, page 3
#define MAX_LINE_LENGTH 512
#define RS_LIMIT 4000
#define RR_LIMIT 5000

typedef struct {
    char address[64];
    int port;
} Endpoint;

typedef struct {
    Endpoint local;
    Endpoint remote;
    int rtp_bw;  // in kbps
    int rs_bw;   // RTCP sender bandwidth in bps
    int rr_bw;   // RTCP receiver bandwidth in bps
    int rtcp_bw; // in bps
} Bandwidth;

void parse_c_line(char *line, Endpoint *endpoint) {
    sscanf(line, "c=IN IP6 %63s", endpoint->address);
}

void parse_m_line(char *line, Endpoint *endpoint) {
    sscanf(line, "m=audio %d", &endpoint->port);
}

void parse_b_line(char *line, const char *type, int *bw) {
    if (strncmp(type, "AS", 2) == 0) {
        sscanf(line, "b=AS:%d", bw);
    } else if (strncmp(type, "RS", 2) == 0) {
        sscanf(line, "b=RS:%d", bw);
    } else if (strncmp(type, "RR", 2) == 0) {
        sscanf(line, "b=RR:%d", bw);
    }
}

void calculate_bandwidth(Bandwidth *bw) {
    if (bw->rs_bw == 0) {
        bw->rs_bw = (int)(SENDER_RTCP_BW_PERCENT * bw->rtp_bw * 1000);
    }
    if (bw->rr_bw == 0) {
        bw->rr_bw = (int)(RECEIVER_RTCP_BW_PERCENT * bw->rtp_bw * 1000);
    }
    if (bw->rs_bw > RS_LIMIT) {
        fprintf(stderr, "Error: RTCP sender bandwidth exceeds the limit of %d bps\n", RS_LIMIT);
        exit(EXIT_FAILURE);
    }
    if (bw->rr_bw > RR_LIMIT) {
        fprintf(stderr, "Error: RTCP receiver bandwidth exceeds the limit of %d bps\n", RR_LIMIT);
        exit(EXIT_FAILURE);
    }
    bw->rtcp_bw = bw->rs_bw + bw->rr_bw;
}

void print_bandwidth(Bandwidth *bw/*, int direction*/) {
    printf("permit %dkbps from %s %d to %s %d\n",
        bw->rtp_bw, bw->local.address, bw->local.port,
        bw->remote.address, bw->remote.port);
    printf("permit %dbps from %s %d to %s %d\n",
        bw->rtcp_bw, bw->local.address, bw->local.port,
        bw->remote.address, bw->remote.port);
}

void print_total(Bandwidth *bw1, Bandwidth *bw2) {
    double total_uplink_1 = bw1->rtp_bw + (double)bw1->rtcp_bw / 1000;
    double total_uplink_2 = bw2->rtp_bw + (double)bw2->rtcp_bw / 1000;
    printf("Total uplink from %s ---> %.1f kbps\n",
        bw1->local.address, total_uplink_1);
    printf("Total uplink from %s ---> %.1f kbps\n",
        bw2->local.address, total_uplink_2);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return EXIT_FAILURE;
    }

    FILE *file = fopen(argv[1], "r");
    if (!file) {
        perror("fopen");
        return EXIT_FAILURE;
    }

    char line[MAX_LINE_LENGTH];
    Endpoint local = {0}, remote = {0};
    int rtp_bw_local = 0, rs_bw_local = 0, rr_bw_local = 0;
    int rtp_bw_remote = 0, rs_bw_remote = 0, rr_bw_remote = 0;
    Bandwidth bw1 = {0}, bw2 = {0};
    bool in_local = false;
    bool ignore_next_c_line = false; // Flag to ignore the second c= line
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "v=0", 3) == 0) {
            in_local = !in_local;
            ignore_next_c_line = false; //reset flag
        } else if (strncmp(line, "c=", 2) == 0) {
            if (in_local){
                parse_c_line(line, &local);
                ignore_next_c_line = true;
        }
            else{
                parse_c_line(line, &remote);
                ignore_next_c_line = true;
        }
        } else if (strncmp(line, "m=", 2) == 0) {
            if (in_local)
                parse_m_line(line, &local);
            else
                parse_m_line(line, &remote);
        } else if (strncmp(line, "b=AS:", 5) == 0) {
            if (in_local)
                parse_b_line(line, "AS", &rtp_bw_local);
            else
                parse_b_line(line, "AS", &rtp_bw_remote);
        } else if (strncmp(line, "b=RS:", 5) == 0) {
            if (in_local)
                parse_b_line(line, "RS", &rs_bw_local);
            else
                parse_b_line(line, "RS", &rs_bw_remote);
        } else if (strncmp(line, "b=RR:", 5) == 0) {
            if (in_local)
                parse_b_line(line, "RR", &rr_bw_local);
            else
                parse_b_line(line, "RR", &rr_bw_remote);
        }
    }

    fclose(file);

    // Set up bandwidth structures and calculate bandwidth
    bw1.local = local;
    bw1.remote = remote;
    bw1.rtp_bw = rtp_bw_local;
    bw1.rs_bw = rs_bw_local;
    bw1.rr_bw = rr_bw_local;
    calculate_bandwidth(&bw1);

    bw2.local = remote;
    bw2.remote = local;
    bw2.rtp_bw = rtp_bw_remote;
    bw2.rs_bw = rs_bw_remote;
    bw2.rr_bw = rr_bw_remote;
    calculate_bandwidth(&bw2);

    // Print bandwidth information and totals
    print_bandwidth(&bw1);  // local to remote
    print_bandwidth(&bw2);  // remote to local
    print_total(&bw1, &bw2);

    return 0;
}
