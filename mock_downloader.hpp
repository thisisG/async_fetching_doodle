#ifndef MOCK_DOWNLOADER_HPP
#define MOCK_DOWNLOADER_HPP

#include <algorithm>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace software {

struct downloader {
    void download(std::string const & product, std::function<void(std::string)> on_done)
    {
        auto & product_cbs{done_cbs[product]};
        product_cbs.emplace_back(on_done);
    }

    bool download_pending(std::string const & product)
    {
        return !done_cbs[product].empty();
    }

    void success(std::string const & product)
    {
        auto & product_cbs{done_cbs[product]};
        std::for_each(std::begin(product_cbs), std::end(product_cbs), [](auto & cb) {
            cb("success");
        });

        product_cbs.clear();
    }

    void failure(std::string const & product, std::string const & reason)
    {
        auto & product_cbs{done_cbs[product]};
        std::for_each(
            std::begin(product_cbs), std::end(product_cbs), [&reason](auto & cb) {
                cb(reason);
            });

        product_cbs.clear();
    }

    std::map<std::string, std::vector<std::function<void(std::string)>>> done_cbs;
};

}
#endif