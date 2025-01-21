#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <iostream>
#include <string>
#include <map>
#include <set>
#include <nlohmann/json.hpp>
#include <curl/curl.h>

using json = nlohmann::json;
using namespace std;
using namespace std::placeholders;
using websocketpp::connection_hdl;

typedef websocketpp::server<websocketpp::config::asio> websocket_server;

size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}

struct connection_comparator
{
    bool operator()(const connection_hdl &h1, const connection_hdl &h2) const
    {
        return !h1.owner_before(h2) && !h2.owner_before(h1);
    }
};

class RestClient
{
public:
    RestClient(const std::string &clientId, const std::string &clientSecret, const std::string &url)
        : clientId(clientId), clientSecret(clientSecret), url(url), token("")
    {
    }

    bool authenticate()
    {
        CURL *curl;
        CURLcode res;
        std::string readBuffer;
        curl = curl_easy_init();
        if (curl)
        {
            // Construct the full URL with query parameters
            std::string fullUrl = url + "/api/v2/public/auth?client_id=" + clientId + "&client_secret=" + clientSecret + "&grant_type=client_credentials";
            curl_easy_setopt(curl, CURLOPT_URL, fullUrl.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

            // Set headers
            struct curl_slist *headers = nullptr;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

            // Perform the request
            res = curl_easy_perform(curl);
            if (res != CURLE_OK)
            {
                std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
                curl_easy_cleanup(curl);
                return false;
            }

            // Parse the response
            json jsonResponse = json::parse(readBuffer);
            if (jsonResponse.contains("result") && jsonResponse["result"].contains("access_token"))
            {
                token = jsonResponse["result"]["access_token"];
                std::cout << "Authenticated. Token: " << token << std::endl;
                curl_easy_cleanup(curl);
                return true;
            }
            curl_easy_cleanup(curl);
        }
        return false;
    }

    json makeRequest(const std::string &endpoint, const std::string &method = "GET", const std::string &queryParams = "")
    {
        CURL *curl;
        CURLcode res;
        std::string readBuffer;
        curl = curl_easy_init();
        if (curl)
        {
            std::string fullUrl = url + endpoint;

            // If it's a GET request and queryParams are provided, append them to the URL
            if (method == "GET" && !queryParams.empty())
            {
                fullUrl += "?" + queryParams;
            }

            curl_easy_setopt(curl, CURLOPT_URL, fullUrl.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

            if (method == "POST")
            {
                curl_easy_setopt(curl, CURLOPT_POST, 1L);
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, queryParams.c_str());
            }

            if (!token.empty())
            {
                struct curl_slist *headers = nullptr;
                headers = curl_slist_append(headers, ("Authorization: Bearer " + token).c_str());
                headers = curl_slist_append(headers, "Content-Type: application/json");
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            }
            cout << "Inside order placed" << endl;
            res = curl_easy_perform(curl);
            if (res != CURLE_OK)
            {
                std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
                curl_easy_cleanup(curl);
                return {};
            }

            curl_easy_cleanup(curl);
        }
        return json::parse(readBuffer);
    }

    json getOrderBook(const std::string &instrument_name, int depth = 5)
    {
        return makeRequest("/api/v2/public/get_order_book?instrument_name=" + instrument_name + "&depth=" + std::to_string(depth));
    }

public:
    std::string clientId;
    std::string clientSecret;
    std::string url;
    std::string token;
};

class OrderBookServer
{
public:
    OrderBookServer() : m_server()
    {
        m_server.init_asio();
        m_server.set_open_handler(bind(&OrderBookServer::on_open, this, _1));
        m_server.set_close_handler(bind(&OrderBookServer::on_close, this, _1));
        m_server.set_message_handler(bind(&OrderBookServer::on_message, this, _1, _2));
    }

    void on_open(connection_hdl hdl)
    {
        m_connections.insert(hdl);
    }

    void on_close(connection_hdl hdl)
    {
        m_connections.erase(hdl);
        for (auto &subscription : m_subscriptions)
        {
            subscription.second.erase(hdl);
        }
    }

    void on_message(connection_hdl hdl, websocket_server::message_ptr msg)
    {
        json message = json::parse(msg->get_payload());
        string type = message["type"];
        string symbol = message["symbol"];

        if (type == "subscribe")
        {
            m_subscriptions[symbol].insert(hdl);
            cout << "Client subscribed to: " << symbol << endl;
        }
        else if (type == "unsubscribe")
        {
            m_subscriptions[symbol].erase(hdl);
            cout << "Client unsubscribed from: " << symbol << endl;
        }
    }

    void run(uint16_t port)
    {
        m_server.listen(port);
        m_server.start_accept();
        m_server.run();
    }

    void send_order_book_update(const string &symbol, const json &update)
    {
        auto it = m_subscriptions.find(symbol);
        if (it != m_subscriptions.end())
        {
            string message = update.dump();
            for (auto hdl : it->second)
            {
                m_server.send(hdl, message, websocketpp::frame::opcode::text);
            }
        }
    }

private:
    websocket_server m_server;
    set<connection_hdl, connection_comparator> m_connections;
    map<string, set<connection_hdl, connection_comparator>> m_subscriptions;
};

int main()
{
    OrderBookServer server;
    thread server_thread([&server]()
                         { server.run(9002); });

    string clientId = "P6TGQTrl";
    string clientSecret = "-_rbnZQOEqNpTuUqRbpY4uOnCKmmwvtYuqT3XYfJ4aA";
    RestClient client(clientId, clientSecret, "https://test.deribit.com");

    while (true)
    {
        this_thread::sleep_for(chrono::seconds(3));
        if (client.authenticate())
        {
            json order_book_update = client.getOrderBook("BTC-PERPETUAL", 5);
            server.send_order_book_update("BTC_USD", order_book_update);
        }
    }

    server_thread.join();
    return 0;
}
