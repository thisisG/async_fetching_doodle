#include "fetcher.hpp"

#include <algorithm>
#include <utility>

namespace software {

std::string to_string(product product)
{
    switch (product) {
    case product::headset:
        return "headset";
    case product::screen:
        return "screen";
    case product::fpga:
        return "fpga";
    }
}

fetcher::fetcher(downloader & downloader)
    : downloader_{downloader}
{
}

void fetcher::fetch(product product, id id, callback && callback)
{
    auto & product_info{fetch_data_[product]};
    product_info.emplace_back(
        callback_info{id, std::forward<fetcher::callback>(callback)});

    downloader_.download(to_string(product), [this, product](auto const & result) {
        handle_download_result(product, result);
    });
}

void fetcher::cancel(id id)
{
    auto erase_cbs_for_id = [id](auto & map) {
        auto & collection = map.second;
        collection.erase(std::remove_if(
            std::begin(collection), std::end(collection), [id](const auto & entry) {
                return entry.id == id;
            }));
    };

    std::for_each(std::begin(fetch_data_), std::end(fetch_data_), erase_cbs_for_id);
}

fetcher::id fetcher::unique_id()
{
    static fetcher::id unique_id{0};
    return ++unique_id;
}

void fetcher::handle_download_result(product product, std::string const & result)
{
    auto & product_info{fetch_data_[product]};

    auto const fetch_result = [&result]() -> fetcher::result {
        if (result == "success"s) {
            return {result::success_t{"/software_location"}, std::nullopt};
        }
        if (result == "missing"s) {
            return {std::nullopt, result::failure_t{result::failure_t::type::missing}};
        }
        if (result == "timeout"s) {
            return {std::nullopt, result::failure_t{result::failure_t::type::timed_out}};
        }
        abort();
    }();

    std::for_each(
        std::begin(product_info),
        std::end(product_info),
        [&fetch_result](auto const & info) {
            // Should probably be invoked asynchronously to avoid blowing up the
            // stack - c'est la vie.
            info.cb({fetch_result});
        });

    product_info.clear();
}

}