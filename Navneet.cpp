#include "secrets.h"
#include <iostream>
#include <string>
#include <curl/curl.h>
#include "json.hpp"

using json = nlohmann::json;
using namespace std;

size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}

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

    json buy(const json &buy_obj)
    {
        std::string queryParams;
        for (auto it = buy_obj.begin(); it != buy_obj.end(); ++it)
        {
            if (it != buy_obj.begin())
            {
                queryParams += "&";
            }

            // If the value is a string, get it directly without quotes
            if (it->is_string())
            {
                queryParams += it.key() + "=" + it->get<std::string>();
            }
            else
            {
                queryParams += it.key() + "=" + it->dump();
            }
        }
        return makeRequest("/api/v2/private/buy", "GET", queryParams);
    }

    json modifyOrder(const json &orderParams)
    {
        std::string queryParams;
        for (auto it = orderParams.begin(); it != orderParams.end(); ++it)
        {
            if (it != orderParams.begin())
            {
                queryParams += "&";
            }

            // If the value is a string, get it directly without quotes
            if (it->is_string())
            {
                queryParams += it.key() + "=" + it->get<std::string>();
            }
            else
            {
                queryParams += it.key() + "=" + it->dump();
            }
        }
        return makeRequest("/api/v2/private/edit", "GET", queryParams);
    }

    json placeOrder(const json &buy_obj)
    {
        return makeRequest("/api/v2/private/buy", "GET", buy_obj.dump());
    }

    json cancelOrder(const std::string &order_id)
    {
        return makeRequest("/api/v2/private/cancel?order_id=" + order_id);
    }

    json getOrderBook(const std::string &instrument_name, int depth = 5)
    {
        return makeRequest("/api/v2/public/get_order_book?instrument_name=" + instrument_name + "&depth=" + std::to_string(depth));
    }

    json getPositions(const json &positionParams)
    {
        return makeRequest("/api/v2/private/get_positions", "GET", positionParams.dump());
    }

    void executeAllFunctions()
    {
        // Example calls
        // // Place an order
        // json buy_obj = {
        //     {"amount", 70},
        //     {"instrument_name", "ADA_USDC-PERPETUAL"},
        //     {"label", "market0000567"},
        //     {"type", "market"}};
        // json response_buy = buy(buy_obj);
        // std::cout << "Order placed: " << response_buy.dump(4) << std::endl;

        // Modify an order
        // json modifyParams = {
        //     {"amount", 101},
        //     {"order_id", "USDC-13386813264"},
        //     {"price", 0.12}};
        // json response_modify = modifyOrder(modifyParams);
        // std::cout << "Order modified: " << response_modify.dump(4) << std::endl;

        // // Cancel an order
        // std::string order_id = "USDC-13386813264";
        // json response_cancel = cancelOrder(order_id);
        // std::cout << "Order canceled: " << response_cancel.dump(4) << std::endl;

        // // Get order book
        // json response_orderbook = getOrderBook("BTC-PERPETUAL", 5);
        // std::cout << "Order Book: " << response_orderbook.dump(4) << std::endl;

        // // Get positions
        // json positionParams = {
        //     {"currency", "BTC"},
        //     {"kind", "future"}};
        // json response_position = getPositions(positionParams);
        // std::cout << "Positions: " << response_position.dump(4) << std::endl;
    }

public:
    std::string clientId;
    std::string clientSecret;
    std::string url;
    std::string token;
};

int main()
{
    RestClient client(CLIENT_ID, CLIENT_SECRET, "https://test.deribit.com");

    if (client.authenticate())
    {
        // Call the function to execute all API interactions
        client.executeAllFunctions();
    }

    return 0;
}