#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

#include <iostream>
#include <nlohmann/json.hpp>
#include <cstdlib>

using json = nlohmann::json;
using namespace std;
using websocketpp::connection_hdl;

typedef websocketpp::client<websocketpp::config::asio_client> client;

class OrderBookClient {
public:
    OrderBookClient(const string& uri) : m_uri(uri) {
        m_client.init_asio();
        m_client.set_open_handler(bind(&OrderBookClient::on_open, this, placeholders::_1));
        m_client.set_message_handler(bind(&OrderBookClient::on_message, this, placeholders::_1, placeholders::_2));
    }

    void on_open(connection_hdl hdl) {
        m_hdl = hdl;
        json message = {
            {"type", "subscribe"},
            {"symbol", "BTC_USD"}
        };
        m_client.send(hdl, message.dump(), websocketpp::frame::opcode::text);
        cout << "Sent subscription request for BTC_USD" << endl;
    }

    void on_message(connection_hdl, client::message_ptr msg) {
        cout << "Received Message: " << msg->get_payload() << endl;
    }

    void run() {
        websocketpp::lib::error_code ec;
        client::connection_ptr con = m_client.get_connection(m_uri, ec);
        if (ec) {
            cout << "Could not create connection because: " << ec.message() << endl;
            return;
        }

        m_client.connect(con);
        m_client.run();
    }

private:
    client m_client;
    string m_uri;
    connection_hdl m_hdl;
};

int main(int argc, char* argv[]) {
    string uri = "ws://localhost:9002";
    if (argc > 1) {
        uri = argv[1];
    }

    OrderBookClient obClient(uri);
    obClient.run();

    return 0;
}
