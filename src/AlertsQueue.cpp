/*
 *
 * (C) 2019 - ntop.org
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include "ntop_includes.h"

/* **************************************************** */

AlertsQueue::AlertsQueue(NetworkInterface *_iface) {
  iface = _iface;
}

/* **************************************************** */

/*
 * Note: consumer should destroy the tlv with:
 * ndpi_term_serializer(tlv);
 * free(tlv);
 */
void AlertsQueue::pushAlertJson(const char *atype, ndpi_serializer *alert) {
  /* These are mandatory fields, present in all the pushed alerts */
  ndpi_serialize_string_uint32(alert, "ifid", iface->get_id());
  ndpi_serialize_string_string(alert, "alert_type", atype);
  ndpi_serialize_string_uint64(alert, "alert_tstamp", time(NULL));

  if(!ntop->getInternalAlertsQueue()->enqueue(alert))
    iface->incNumDroppedAlerts(1);
}

/* **************************************************** */

void AlertsQueue::pushOutsideDhcpRangeAlert(u_int8_t *cli_mac, Mac *sender_mac,
    u_int32_t ip, u_int32_t router_ip, int vlan_id) {
  ndpi_serializer *tlv;

  if(ntop->getPrefs()->are_alerts_disabled())
    return;

  tlv = (ndpi_serializer *) calloc(1, sizeof(ndpi_serializer));

  if (tlv) {
    char cli_mac_s[32], sender_mac_s[32];
    char ipbuf[64], router_ip_buf[64], *ip_s, *router_ip_s;

    Utils::formatMac(cli_mac, cli_mac_s, sizeof(cli_mac_s));
    sender_mac->print(sender_mac_s, sizeof(cli_mac_s));
    ip_s = Utils::intoaV4(ip, ipbuf, sizeof(ipbuf));
    router_ip_s = Utils::intoaV4(router_ip, router_ip_buf, sizeof(router_ip_buf));

    ntop->getTrace()->traceEvent(TRACE_INFO, "IP not in DHCP range: %s (mac=%s, sender=%s, router=%s)",
				       ipbuf, cli_mac_s, sender_mac_s, router_ip_s);

    ndpi_init_serializer_ll(tlv, ndpi_serialization_format_tlv, 64);

    ndpi_serialize_string_string(tlv, "client_mac", cli_mac_s);
    ndpi_serialize_string_string(tlv, "sender_mac", sender_mac_s);
    ndpi_serialize_string_string(tlv, "client_ip", ip_s);
    ndpi_serialize_string_string(tlv, "router_ip", router_ip_s);
    ndpi_serialize_string_int32(tlv, "vlan_id", vlan_id);

    pushAlertJson("misconfigured_dhcp_range", tlv);
  }
}

/* **************************************************** */

void AlertsQueue::pushSlowPeriodicActivity(u_long msec_diff,
    u_long max_duration_ms, const char *activity_path) {
  ndpi_serializer *tlv;

  if(ntop->getPrefs()->are_alerts_disabled())
    return;

  tlv = (ndpi_serializer *) calloc(1, sizeof(ndpi_serializer));

  if (tlv) {
    ndpi_init_serializer_ll(tlv, ndpi_serialization_format_tlv, 64);

    ndpi_serialize_string_int64(tlv, "duration_ms", msec_diff);
    ndpi_serialize_string_int64(tlv, "max_duration_ms", max_duration_ms);
    ndpi_serialize_string_string(tlv, "path", activity_path);

    pushAlertJson("slow_periodic_activity", tlv);
  }
}

/* **************************************************** */

void AlertsQueue::pushMacIpAssociationChangedAlert(u_int32_t ip, u_int8_t *old_mac, u_int8_t *new_mac) {
  ndpi_serializer *tlv;

  if(ntop->getPrefs()->are_alerts_disabled())
    return;

  tlv = (ndpi_serializer *) calloc(1, sizeof(ndpi_serializer));

  if (tlv) {
    char oldmac_s[32], newmac_s[32], ipbuf[32], *ip_s;

    Utils::formatMac(old_mac, oldmac_s, sizeof(oldmac_s));
    Utils::formatMac(new_mac, newmac_s, sizeof(newmac_s));
    ip_s = Utils::intoaV4(ip, ipbuf, sizeof(ipbuf));

    ntop->getTrace()->traceEvent(TRACE_INFO, "IP %s: modified MAC association %s -> %s",
				       ip_s, oldmac_s, newmac_s);

    ndpi_init_serializer_ll(tlv, ndpi_serialization_format_tlv, 64);

    ndpi_serialize_string_string(tlv, "ip", ip_s);
    ndpi_serialize_string_string(tlv, "old_mac", oldmac_s);
    ndpi_serialize_string_string(tlv, "new_mac", newmac_s);

    pushAlertJson("mac_ip_association_change", tlv);
  }
}

/* **************************************************** */

void AlertsQueue::pushBroadcastDomainTooLargeAlert(const u_int8_t *src_mac, const u_int8_t *dst_mac,
    u_int32_t spa, u_int32_t tpa, int vlan_id) {
  ndpi_serializer *tlv;

  if(ntop->getPrefs()->are_alerts_disabled())
    return;

  tlv = (ndpi_serializer *) calloc(1, sizeof(ndpi_serializer));

  if (tlv) {
    char src_mac_s[32], dst_mac_s[32], spa_buf[32], tpa_buf[32];
    char *spa_s, *tpa_s;

    Utils::formatMac(src_mac, src_mac_s, sizeof(src_mac_s));
    Utils::formatMac(dst_mac, dst_mac_s, sizeof(dst_mac_s));
    spa_s = Utils::intoaV4(spa, spa_buf, sizeof(spa_buf));
    tpa_s = Utils::intoaV4(tpa, tpa_buf, sizeof(tpa_buf));

    ndpi_init_serializer_ll(tlv, ndpi_serialization_format_tlv, 64);

    ndpi_serialize_string_int32(tlv, "vlan_id", vlan_id);
    ndpi_serialize_string_string(tlv, "src_mac", src_mac_s);
    ndpi_serialize_string_string(tlv, "dst_mac", dst_mac_s);
    ndpi_serialize_string_string(tlv, "spa", spa_s);
    ndpi_serialize_string_string(tlv, "tpa", tpa_s);

    pushAlertJson("broadcast_domain_too_large", tlv);
  }
}

/* **************************************************** */

void AlertsQueue::pushRemoteToRemoteAlert(Host *host) {
  ndpi_serializer *tlv;

  if(ntop->getPrefs()->are_alerts_disabled())
    return;

  tlv = (ndpi_serializer *) calloc(1, sizeof(ndpi_serializer));

  if (tlv) {
    char ipbuf[64], macbuf[32];

    ndpi_init_serializer_ll(tlv, ndpi_serialization_format_tlv, 64);

    ndpi_serialize_string_string(tlv, "host", host->get_ip()->print(ipbuf, sizeof(ipbuf)));
    ndpi_serialize_string_int32(tlv, "vlan", host->get_vlan_id());
    ndpi_serialize_string_string(tlv, "mac_address", host->getMac() ? host->getMac()->print(macbuf, sizeof(macbuf)) : "");

    pushAlertJson("remote_to_remote", tlv);
  }
}

/* **************************************************** */

void AlertsQueue::pushLoginTrace(const char*user, bool authorized) {
  ndpi_serializer *tlv;

  if(ntop->getPrefs()->are_alerts_disabled())
    return;

  tlv = (ndpi_serializer *) calloc(1, sizeof(ndpi_serializer));

  if (tlv) {
    ndpi_init_serializer_ll(tlv, ndpi_serialization_format_tlv, 64);

    ndpi_serialize_string_string(tlv, "scope", "login");
    ndpi_serialize_string_string(tlv, "user", user);

    pushAlertJson(authorized ? "user_activity" : "login_failed", tlv);
  }
}

/* **************************************************** */

void AlertsQueue::pushNfqFlushedAlert(int queue_len, int queue_len_pct, int queue_dropped) {
  ndpi_serializer *tlv;

  if(ntop->getPrefs()->are_alerts_disabled())
    return;

  tlv = (ndpi_serializer *) calloc(1, sizeof(ndpi_serializer));

  if (tlv) {
    ndpi_init_serializer_ll(tlv, ndpi_serialization_format_tlv, 64);

    ndpi_serialize_string_int32(tlv, "tot",     queue_len);
    ndpi_serialize_string_int32(tlv, "pct",     queue_len_pct);
    ndpi_serialize_string_int32(tlv, "dropped", queue_dropped);

    pushAlertJson("nfq_flushed", tlv);
  }
}
