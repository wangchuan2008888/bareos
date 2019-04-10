/*
   BAREOS® - Backup Archiving REcovery Open Sourced

   Copyright (C) 2004-2011 Free Software Foundation Europe e.V.
   Copyright (C) 2011-2012 Planets Communications B.V.
   Copyright (C) 2013-2016 Bareos GmbH & Co. KG

   This program is Free Software; you can redistribute it and/or
   modify it under the terms of version three of the GNU Affero General Public
   License as published by the Free Software Foundation and included
   in the file LICENSE.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.
*/
/*
 * Written by Meno Abels, June MMIV
 */
/**
 * @file
 * Configuration file parser for IP-Addresse ipv4 and ipv6
 */

#include "include/bareos.h"
#include "lib/address_conf.h"
#include "lib/bnet.h"
#include "lib/bsys.h"
#include "lib/edit.h"


#ifdef HAVE_ARPA_NAMESER_H
#include <arpa/nameser.h>
#endif
#ifdef HAVE_RESOLV_H
//#include <resolv.h>
#endif

IPADDR::IPADDR()
    : type(R_UNDEFINED)
    , saddr(nullptr)
    , saddr4(nullptr)
#ifdef HAVE_IPV6
    , saddr6(nullptr)
#endif
{
  memset(&saddrbuf, 0, sizeof(saddrbuf));
}

IPADDR::IPADDR(const IPADDR& src) : IPADDR()
{
  type = src.type;
  memcpy(&saddrbuf, &src.saddrbuf, sizeof(saddrbuf));
  saddr = &saddrbuf.dontuse;
  saddr4 = &saddrbuf.dontuse4;
#ifdef HAVE_IPV6
  saddr6 = &saddrbuf.dontuse6;
#endif
}

IPADDR::IPADDR(int af) : IPADDR()
{
  type = R_EMPTY;
#ifdef HAVE_IPV6
  if (!(af == AF_INET6 || af == AF_INET)) {
    Emsg1(M_ERROR_TERM, 0, _("Only ipv4 and ipv6 are supported (%d)\n"), af);
  }
#else
  if (af != AF_INET) {
    Emsg1(M_ERROR_TERM, 0, _("Only ipv4 is supported (%d)\n"), af);
  }
#endif

  memset(&saddrbuf, 0, sizeof(saddrbuf));
  saddr = &saddrbuf.dontuse;
  saddr4 = &saddrbuf.dontuse4;
#ifdef HAVE_IPV6
  saddr6 = &saddrbuf.dontuse6;
#endif
  saddr->sa_family = af;
  switch (af) {
    case AF_INET:
      saddr4->sin_port = 0xffff;
#ifdef HAVE_SA_LEN
      saddr->sa_len = sizeof(sockaddr_in);
#endif
      break;
#ifdef HAVE_IPV6
    case AF_INET6:
      saddr6->sin6_port = 0xffff;
#ifdef HAVE_SA_LEN
      saddr->sa_len = sizeof(sockaddr_in6);
#endif
      break;
#endif
  }

  SetAddrAny();
}

void IPADDR::SetType(i_type o) { type = o; }

IPADDR::i_type IPADDR::GetType() const { return type; }

unsigned short IPADDR::GetPortNetOrder() const
{
  unsigned short port = 0;
  if (saddr->sa_family == AF_INET) {
    port = saddr4->sin_port;
  }
#ifdef HAVE_IPV6
  else {
    port = saddr6->sin6_port;
  }
#endif
  return port;
}

void IPADDR::SetPortNet(unsigned short port)
{
  if (saddr->sa_family == AF_INET) {
    saddr4->sin_port = port;
  }
#ifdef HAVE_IPV6
  else {
    saddr6->sin6_port = port;
  }
#endif
}

int IPADDR::GetFamily() const { return saddr->sa_family; }

struct sockaddr* IPADDR::get_sockaddr() { return saddr; }

int IPADDR::GetSockaddrLen()
{
#ifdef HAVE_IPV6
  return saddr->sa_family == AF_INET ? sizeof(*saddr4) : sizeof(*saddr6);
#else
  return sizeof(*saddr4);
#endif
}
void IPADDR::CopyAddr(IPADDR* src)
{
  if (saddr->sa_family == AF_INET) {
    saddr4->sin_addr.s_addr = src->saddr4->sin_addr.s_addr;
  }
#ifdef HAVE_IPV6
  else {
    saddr6->sin6_addr = src->saddr6->sin6_addr;
  }
#endif
}

void IPADDR::SetAddrAny()
{
  if (saddr->sa_family == AF_INET) {
    saddr4->sin_addr.s_addr = INADDR_ANY;
  }
#ifdef HAVE_IPV6
  else {
    saddr6->sin6_addr = in6addr_any;
  }
#endif
}

void IPADDR::SetAddr4(struct in_addr* ip4)
{
  if (saddr->sa_family != AF_INET) {
    Emsg1(M_ERROR_TERM, 0,
          _("It was tried to assign a ipv6 address to a ipv4(%d)\n"),
          saddr->sa_family);
  }
  saddr4->sin_addr = *ip4;
}

#ifdef HAVE_IPV6
void IPADDR::SetAddr6(struct in6_addr* ip6)
{
  if (saddr->sa_family != AF_INET6) {
    Emsg1(M_ERROR_TERM, 0,
          _("It was tried to assign a ipv4 address to a ipv6(%d)\n"),
          saddr->sa_family);
  }
  saddr6->sin6_addr = *ip6;
}
#endif

const char* IPADDR::GetAddress(char* outputbuf, int outlen)
{
  outputbuf[0] = '\0';
#ifdef HAVE_INET_NTOP
#ifdef HAVE_IPV6
  inet_ntop(saddr->sa_family,
            saddr->sa_family == AF_INET ? (void*)&(saddr4->sin_addr)
                                        : (void*)&(saddr6->sin6_addr),
            outputbuf, outlen);
#else
  inet_ntop(saddr->sa_family, (void*)&(saddr4->sin_addr), outputbuf, outlen);
#endif
#else
  bstrncpy(outputbuf, inet_ntoa(saddr4->sin_addr), outlen);
#endif
  return outputbuf;
}

const char* IPADDR::BuildConfigString(char* buf, int blen)
{
  char tmp[1024];

  switch (GetFamily()) {
    case AF_INET:
      Bsnprintf(buf, blen,
                "      ipv4 = {\n"
                "         addr = %s\n"
                "         port = %hu\n"
                "      }",
                GetAddress(tmp, sizeof(tmp) - 1), GetPortHostOrder());
      break;
    case AF_INET6:
      Bsnprintf(buf, blen,
                "      ipv6 = {\n"
                "         addr = %s\n"
                "         port = %hu\n"
                "      }",
                GetAddress(tmp, sizeof(tmp) - 1), GetPortHostOrder());
      break;
    default:
      break;
  }

  return buf;
}

const char* IPADDR::build_address_str(char* buf,
                                      int blen,
                                      bool print_port /*=true*/)
{
  char tmp[1024];
  if (print_port) {
    switch (GetFamily()) {
      case AF_INET:
        Bsnprintf(buf, blen, "host[ipv4;%s;%hu] ",
                  GetAddress(tmp, sizeof(tmp) - 1), GetPortHostOrder());
        break;
      case AF_INET6:
        Bsnprintf(buf, blen, "host[ipv6;%s;%hu] ",
                  GetAddress(tmp, sizeof(tmp) - 1), GetPortHostOrder());
        break;
      default:
        break;
    }
  } else {
    switch (GetFamily()) {
      case AF_INET:
        Bsnprintf(buf, blen, "host[ipv4;%s] ",
                  GetAddress(tmp, sizeof(tmp) - 1));
        break;
      case AF_INET6:
        Bsnprintf(buf, blen, "host[ipv6;%s] ",
                  GetAddress(tmp, sizeof(tmp) - 1));
        break;
      default:
        break;
    }
  }

  return buf;
}

const char* BuildAddressesString(dlist* addrs,
                                 char* buf,
                                 int blen,
                                 bool print_port /*=true*/)
{
  if (!addrs || addrs->size() == 0) {
    bstrncpy(buf, "", blen);
    return buf;
  }
  char* work = buf;
  IPADDR* p;
  foreach_dlist (p, addrs) {
    char tmp[1024];
    int len = Bsnprintf(work, blen, "%s",
                        p->build_address_str(tmp, sizeof(tmp), print_port));
    if (len < 0) break;
    work += len;
    blen -= len;
  }
  return buf;
}

const char* GetFirstAddress(dlist* addrs, char* outputbuf, int outlen)
{
  return ((IPADDR*)(addrs->first()))->GetAddress(outputbuf, outlen);
}

int GetFirstPortNetOrder(dlist* addrs)
{
  if (!addrs) {
    return 0;
  } else {
    return ((IPADDR*)(addrs->first()))->GetPortNetOrder();
  }
}

int GetFirstPortHostOrder(dlist* addrs)
{
  if (!addrs) {
    return 0;
  } else {
    return ((IPADDR*)(addrs->first()))->GetPortHostOrder();
  }
}

int AddAddress(dlist** out,
               IPADDR::i_type type,
               unsigned short defaultport,
               int family,
               const char* hostname_str,
               const char* port_str,
               char* buf,
               int buflen)
{
  IPADDR* iaddr;
  IPADDR* jaddr;
  dlist* hostaddrs;
  unsigned short port;
  IPADDR::i_type intype = type;

  buf[0] = 0;
  dlist* addrs = (dlist*)(*(out));
  if (!addrs) {
    IPADDR* tmp = 0;
    addrs = *out = New(dlist(tmp, &tmp->link));
  }

  type = (type == IPADDR::R_SINGLE_PORT || type == IPADDR::R_SINGLE_ADDR)
             ? IPADDR::R_SINGLE
             : type;
  if (type != IPADDR::R_DEFAULT) {
    IPADDR* def = 0;
    foreach_dlist (iaddr, addrs) {
      if (iaddr->GetType() == IPADDR::R_DEFAULT) {
        def = iaddr;
      } else if (iaddr->GetType() != type) {
        Bsnprintf(buf, buflen,
                  _("the old style addresses cannot be mixed with new style"));
        return 0;
      }
    }
    if (def) {
      addrs->remove(def);
      delete def;
    }
  }

  if (!port_str || port_str[0] == '\0') {
    port = defaultport;
  } else {
    int pnum = atol(port_str);
    if (0 < pnum && pnum < 0xffff) {
      port = htons(pnum);
    } else {
      struct servent* s = getservbyname(port_str, "tcp");
      if (s) {
        port = s->s_port;
      } else {
        Bsnprintf(buf, buflen, _("can't resolve service(%s)"), port_str);
        return 0;
      }
    }
  }

  const char* myerrstr;
  hostaddrs = BnetHost2IpAddrs(hostname_str, family, &myerrstr);
  if (!hostaddrs) {
    Bsnprintf(buf, buflen, _("can't resolve hostname(%s) %s"), hostname_str,
              myerrstr);
    return 0;
  }

  if (intype == IPADDR::R_SINGLE_PORT || intype == IPADDR::R_SINGLE_ADDR) {
    IPADDR* addr;
    if (addrs->size()) {
      addr = (IPADDR*)addrs->first();
    } else {
      addr = New(IPADDR(family));
      addr->SetType(type);
      addr->SetPortNet(defaultport);
      addr->SetAddrAny();
      addrs->append(addr);
    }
    if (intype == IPADDR::R_SINGLE_PORT) { addr->SetPortNet(port); }
    if (intype == IPADDR::R_SINGLE_ADDR) {
      addr->CopyAddr((IPADDR*)(hostaddrs->first()));
    }
  } else {
    foreach_dlist (iaddr, hostaddrs) {
      IPADDR* clone;
      /* for duplicates */
      foreach_dlist (jaddr, addrs) {
        if (iaddr->GetSockaddrLen() == jaddr->GetSockaddrLen() &&
            !memcmp(iaddr->get_sockaddr(), jaddr->get_sockaddr(),
                    iaddr->GetSockaddrLen())) {
          goto skip; /* no price */
        }
      }
      clone = New(IPADDR(*iaddr));
      clone->SetType(type);
      clone->SetPortNet(port);
      addrs->append(clone);
    skip:
      continue;
    }
  }
  FreeAddresses(hostaddrs);
  return 1;
}

void InitDefaultAddresses(dlist** out, const char* port)
{
  char buf[1024];
  unsigned short sport = str_to_int32(port);

  if (!AddAddress(out, IPADDR::R_DEFAULT, htons(sport), AF_INET, 0, 0, buf,
                  sizeof(buf))) {
    Emsg1(M_ERROR_TERM, 0, _("Can't add default address (%s)\n"), buf);
  }
}

void FreeAddresses(dlist* addrs)
{
  while (!addrs->empty()) {
    IPADDR* ptr = (IPADDR*)addrs->first();
    addrs->remove(ptr);
    delete ptr;
  }
  delete addrs;
}

int SockaddrGetPortNetOrder(const struct sockaddr* client_addr)
{
  if (client_addr->sa_family == AF_INET) {
    return ((struct sockaddr_in*)client_addr)->sin_port;
  }
#ifdef HAVE_IPV6
  else {
    return ((struct sockaddr_in6*)client_addr)->sin6_port;
  }
#endif
  return -1;
}

int SockaddrGetPort(const struct sockaddr* client_addr)
{
  if (client_addr->sa_family == AF_INET) {
    return ntohs(((struct sockaddr_in*)client_addr)->sin_port);
  }
#ifdef HAVE_IPV6
  else {
    return ntohs(((struct sockaddr_in6*)client_addr)->sin6_port);
  }
#endif
  return -1;
}

char* SockaddrToAscii(const struct sockaddr* sa, char* buf, int len)
{
#ifdef HAVE_INET_NTOP
  /* MA Bug 5 the problem was that i mixed up sockaddr and in_addr */
  inet_ntop(sa->sa_family,
#ifdef HAVE_IPV6
            sa->sa_family == AF_INET
                ? (void*)&(((struct sockaddr_in*)sa)->sin_addr)
                : (void*)&(((struct sockaddr_in6*)sa)->sin6_addr),
#else
            (void*)&(((struct sockaddr_in*)sa)->sin_addr),
#endif /* HAVE_IPV6 */
            buf, len);
#else
  bstrncpy(buf, inet_ntoa(((struct sockaddr_in*)sa)->sin_addr), len);
#endif
  return buf;
}

#ifdef HAVE_OLD_SOCKOPT
int inet_aton(const char* cp, struct in_addr* inp)
{
  struct in_addr inaddr;

  if ((inaddr.s_addr = InetAddr(cp)) != INADDR_NONE) {
    inp->s_addr = inaddr.s_addr;
    return 1;
  }
  return 0;
}
#endif
