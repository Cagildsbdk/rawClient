#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

unsigned short ip_cksum(unsigned short *, int);
unsigned short udp_check(struct udphdr *, unsigned short, unsigned long,
                         unsigned long);

struct my_packet {
  struct iphdr ip;
  struct udphdr udp;
  char data[8];
};

int main() {
  int sockfd, one = 1;
  unsigned short local_port = htons(3000), syn_port = htons(5000);
  unsigned int size, syn_daddr = inet_addr("127.0.0.1"),
                     syn_saddr = inet_addr("127.0.0.1");
  unsigned char in[65000];
  struct my_packet out;
  struct sockaddr_in to, from;
  to.sin_addr.s_addr = syn_daddr;
  to.sin_family = AF_INET;
  to.sin_port = syn_port;
  from.sin_addr.s_addr = syn_saddr;
  from.sin_family = AF_INET;
  from.sin_port = local_port;

  if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_UDP)) < 0) {
    perror("socket");
    return 1;
  }
  bind(sockfd, (struct sockaddr *)&from, sizeof(from));

  strcpy(out.data, "testing");
  out.ip.version = 4;
  out.ip.ihl = 5;
  out.ip.tos = 0;
  out.ip.tot_len = htons(sizeof(out));
  out.ip.id = getpid();
  out.ip.frag_off = 0;
  out.ip.ttl = 255;
  out.ip.protocol = IPPROTO_UDP;
  out.ip.check = 0;
  out.ip.saddr = syn_saddr;
  out.ip.daddr = syn_daddr;
  out.ip.check = ip_cksum((unsigned short *)&out.ip, 20);
  out.udp.source = local_port;
  out.udp.dest = syn_port;
  out.udp.len = htons(sizeof(struct udphdr) + 8);
  out.udp.check = 0;
  out.udp.check = udp_check(&out.udp, sizeof(struct udphdr) + 8, out.ip.saddr,
                            out.ip.daddr);

  if (setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) == -1) {
    perror("setsockopt");
    return 1;
  }

  if (sendto(sockfd, &out, sizeof(out), 0, (struct sockaddr *)&to, sizeof(to)) <
      0) {
    perror("sendto");
    return 1;
  }
  printf("%d bytes packet sent to ...\n", (int)sizeof(out));

  if (recvfrom(sockfd, &in, 4096, 0, (struct sockaddr *)&from, &size) < 0)
    perror("recvfrom");

  return 0;
}

unsigned short ip_cksum(unsigned short *buff, int len) {
  unsigned long sum = 0;
  while (len > 1) {
    sum += *buff++;
    len -= 2;
  }
  if (len == 1)
    sum += (*buff & 0xff);
  sum = (sum >> 16) + (sum & 0xffff);
  sum += (sum >> 16);
  sum = ~sum;
  return (sum & 0xffff);
}

unsigned short udp_check(struct udphdr *th, unsigned short len,
                         unsigned long saddr, unsigned long daddr) {
  unsigned long sum = 0;
  unsigned short *buff;
  buff = (unsigned short *)&saddr;
  sum += *buff++;
  sum += *buff;
  buff = (unsigned short *)&daddr;
  sum += *buff++;
  sum += *buff;
  sum += IPPROTO_UDP * 256;
  sum += htons(len) & 0xffff;
  buff = (unsigned short *)th;
  while (len > 1) {
    sum += *buff++;
    len -= 2;
  }
  if (len == 1)
    sum += (*buff & 0xff);
  sum = (sum >> 16) + (sum & 0xffff);
  sum += (sum >> 16);
  return ((~sum) & 0xffff);
}
