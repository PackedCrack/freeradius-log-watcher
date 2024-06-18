//
// Created by hackerman on 2/4/24.
//

#pragma once


class CParserBuilder;

class CParser
{
    friend CParserBuilder;
public:
    void parse_file(std::ifstream& file, std::ifstream::pos_type startPosition = 0) const;
private:
    std::unordered_set<std::string> m_Headers;
};


class CParserBuilder
{
public:
    [[nodiscard]] inline CParserBuilder& acct_status_type(){ m_Parser.m_Headers.emplace("Acct-Status-Type"); return *this; }
    [[nodiscard]] inline CParserBuilder& acct_authentic(){ m_Parser.m_Headers.emplace("Acct-Authentic"); return *this; }
    [[nodiscard]] inline CParserBuilder& user_name(){ m_Parser.m_Headers.emplace("User-Name"); return *this; }
    [[nodiscard]] inline CParserBuilder& nas_ip_address(){ m_Parser.m_Headers.emplace("NAS-IP-Address"); return *this; }
    [[nodiscard]] inline CParserBuilder& nas_identifier(){ m_Parser.m_Headers.emplace("NAS-Identifier"); return *this; }
    [[nodiscard]] inline CParserBuilder& called_station_id(){ m_Parser.m_Headers.emplace("Called-Station-Id"); return *this; }
    [[nodiscard]] inline CParserBuilder& nas_port_type(){ m_Parser.m_Headers.emplace("NAS-Port-Type"); return *this; }
    [[nodiscard]] inline CParserBuilder& service_type(){ m_Parser.m_Headers.emplace("Service-Type"); return *this; }
    [[nodiscard]] inline CParserBuilder& nas_port(){ m_Parser.m_Headers.emplace("NAS-Port"); return *this; }
    [[nodiscard]] inline CParserBuilder& calling_station_id(){ m_Parser.m_Headers.emplace("Calling-Station-Id"); return *this; }
    [[nodiscard]] inline CParserBuilder& connect_info(){ m_Parser.m_Headers.emplace("Connect-Info"); return *this; }
    [[nodiscard]] inline CParserBuilder& acct_session_id(){ m_Parser.m_Headers.emplace("Acct-Session-Id"); return *this; }
    [[nodiscard]] inline CParserBuilder& mobility_domain_id(){ m_Parser.m_Headers.emplace("Mobility-Domain-Id"); return *this; }
    [[nodiscard]] inline CParserBuilder& wlan_pairwise_cipher(){ m_Parser.m_Headers.emplace("WLAN-Pairwise-Cipher"); return *this; }
    [[nodiscard]] inline CParserBuilder& wlan_group_cipher(){ m_Parser.m_Headers.emplace("WLAN-Group-Cipher"); return *this; }
    [[nodiscard]] inline CParserBuilder& wlan_akm_suite(){ m_Parser.m_Headers.emplace("WLAN-AKM-Suite"); return *this; }
    [[nodiscard]] inline CParserBuilder& wlan_group_mgmt_cipher(){ m_Parser.m_Headers.emplace("WLAN-Group-Mgmt-Cipher"); return *this; }
    [[nodiscard]] inline CParserBuilder& event_timestamp(){ m_Parser.m_Headers.emplace("Event-Timestamp"); return *this; }
    [[nodiscard]] inline CParserBuilder& acct_delay_time(){ m_Parser.m_Headers.emplace("Acct-Delay-Time"); return *this; }
    [[nodiscard]] inline CParserBuilder& acct_session_time(){ m_Parser.m_Headers.emplace("Acct-Session-Time"); return *this; }
    [[nodiscard]] inline CParserBuilder& acct_input_packets(){ m_Parser.m_Headers.emplace("Acct-Input-Packets"); return *this; }
    [[nodiscard]] inline CParserBuilder& acct_output_packets(){ m_Parser.m_Headers.emplace("Acct-Output-Packets"); return *this; }
    [[nodiscard]] inline CParserBuilder& acct_input_octets(){ m_Parser.m_Headers.emplace("Acct-Input-Octets"); return *this; }
    [[nodiscard]] inline CParserBuilder& acct_input_gigawords(){ m_Parser.m_Headers.emplace("Acct-Input-Gigawords"); return *this; }
    [[nodiscard]] inline CParserBuilder& acct_output_octets(){ m_Parser.m_Headers.emplace("Acct-Output-Octets"); return *this; }
    [[nodiscard]] inline CParserBuilder& acct_output_gigawords(){ m_Parser.m_Headers.emplace("Acct-Output-Gigawords"); return *this; }
    [[nodiscard]] inline CParserBuilder& acct_terminate_cause(){ m_Parser.m_Headers.emplace("Acct-Terminate-Cause"); return *this; }
    [[nodiscard]] inline CParserBuilder& acct_unique_session_id(){ m_Parser.m_Headers.emplace("Acct-Unique-Session-Id"); return *this; }
    [[nodiscard]] inline CParserBuilder& timestamp(){ m_Parser.m_Headers.emplace("Timestamp"); return *this; }
    [[nodiscard]] inline CParser build(){ return CParser{ std::move(m_Parser) }; }
private:
    CParser m_Parser{};
};