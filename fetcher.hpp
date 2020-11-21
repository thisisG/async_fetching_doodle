#ifndef FETCHER_HPP
#define FETCHER_HPP

#include "mock_downloader.hpp"

#include <filesystem>
#include <optional>
#include <vector>

namespace software {

using namespace std::string_literals;

enum class product { headset, screen, fpga };

std::string to_string(product product);

class fetcher {
public:
    struct result {
        struct success_t {
            std::filesystem::path location;
        };
        struct failure_t {
            enum class type { missing, timed_out };
            type type;
        };
        std::optional<success_t> success;
        std::optional<failure_t> failure;
    };

    using callback = std::function<void(result)>;
    using id = uint64_t;

    explicit fetcher(downloader & downloader);

    /* Generates a unique id for starting and cancelling fetching of products. */
    static id unique_id();

    /* Fetch a product. The fetch can be cancelled using the id. callback is called only
     * once with either success or failure when the fetch operation completes*/
    void fetch(product product, id id, callback && callback);

    /* Cancel any fetch for id. Callbacks for the fetch will not be called. */
    void cancel(id id);

private:
    struct callback_info {
        id id;
        callback cb;
    };

    downloader & downloader_;
    std::map<software::product, std::vector<callback_info>> fetch_data_;

    void handle_download_result(product product, std::string const & result);
};

}

#endif
