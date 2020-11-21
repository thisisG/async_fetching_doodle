#define BOOST_TEST_MODULE async_fetcher tests
#include <boost/test/included/unit_test.hpp>

#include "fetcher.hpp"

#include <filesystem>
#include <memory>

namespace {

using namespace software;
namespace fs = std::filesystem;

struct fixture {
    downloader downloader_{};
    fetcher fetcher_{downloader_};
};

BOOST_AUTO_TEST_CASE(fetcher_has_success_and_failure_result_types)
{
    auto result{fetcher::result{}};
    BOOST_TEST(!result.success);
    BOOST_TEST(!result.failure);
}

BOOST_FIXTURE_TEST_SUITE(fetcher_api, fixture)

BOOST_AUTO_TEST_CASE(can_start_fetch_and_cancel)
{
    auto id{fetcher::unique_id()};
    auto product{product::headset};
    auto share_counter = std::make_shared<int>(0);

    fetcher_.fetch(
        product, id, fetcher::callback{[share_counter](auto const & /* result */) {}});

    BOOST_TEST(share_counter.use_count() == 2);

    fetcher_.cancel(id);

    BOOST_TEST(share_counter.use_count() == 1);
}

BOOST_AUTO_TEST_CASE(fetch_for_product_starts_download)
{
    auto id{fetcher::unique_id()};
    auto product{product::headset};

    fetcher_.fetch(product, id, fetcher::callback{[](auto const & /* result */) {}});

    BOOST_TEST(downloader_.download_pending("headset"));
}

BOOST_AUTO_TEST_CASE(success_download_provides_success_result)
{
    auto id{fetcher::unique_id()};
    auto product{product::headset};

    auto called{false};
    fetcher_.fetch(product, id, fetcher::callback{[&called](auto const & result) {
                       called = true;
                       BOOST_TEST(result.success.has_value());
                       auto actual_path{result.success->location.string()};
                       BOOST_TEST(actual_path == std::string("/software_location"));
                   }});

    BOOST_TEST(downloader_.download_pending("headset"));

    downloader_.success("headset");

    BOOST_TEST(called);
    BOOST_TEST(!downloader_.download_pending("headset"));
}

BOOST_AUTO_TEST_CASE(missing_download_provides_missing_result)
{
    auto id{fetcher::unique_id()};
    auto product{product::headset};

    auto called{false};
    fetcher_.fetch(
        product, id, fetcher::callback{[&called](auto const & result) {
            called = true;
            BOOST_TEST(result.failure.has_value());
            BOOST_TEST(
                (result.failure->type == fetcher::result::failure_t::type::missing));
        }});

    BOOST_TEST(downloader_.download_pending("headset"));

    downloader_.failure("headset", "missing");

    BOOST_TEST(called);
    BOOST_TEST(!downloader_.download_pending("headset"));
}

BOOST_AUTO_TEST_CASE(timeout_download_provides_timeout_result)
{
    auto id{fetcher::unique_id()};
    auto product{product::headset};

    auto called{false};
    fetcher_.fetch(
        product, id, fetcher::callback{[&called](auto const & result) {
            called = true;
            BOOST_TEST(result.failure.has_value());
            BOOST_TEST(
                (result.failure->type == fetcher::result::failure_t::type::timed_out));
        }});

    BOOST_TEST(downloader_.download_pending("headset"));

    downloader_.failure("headset", "timeout");

    BOOST_TEST(called);
    BOOST_TEST(!downloader_.download_pending("headset"));
}

BOOST_AUTO_TEST_CASE(multiple_fetches_calls_all_callbacks_on_success)
{
    auto const product{product::headset};

    auto called_1{false};
    fetcher_.fetch(
        product,
        fetcher::unique_id(),
        fetcher::callback{[&called_1](auto const & /* result */) { called_1 = true; }});

    auto called_2{false};
    fetcher_.fetch(
        product,
        fetcher::unique_id(),
        fetcher::callback{[&called_2](auto const & /* result */) { called_2 = true; }});

    downloader_.success("headset");

    BOOST_TEST(called_1);
    BOOST_TEST(called_2);
}

BOOST_AUTO_TEST_CASE(multiple_fetches_for_same_product_calls_all_callbacks_on_failure)
{
    auto product{product::headset};

    auto called_1{false};
    fetcher_.fetch(
        product,
        fetcher::unique_id(),
        fetcher::callback{[&called_1](auto const & /* result */) { called_1 = true; }});

    auto called_2{false};
    fetcher_.fetch(
        product,
        fetcher::unique_id(),
        fetcher::callback{[&called_2](auto const & /* result */) { called_2 = true; }});

    downloader_.failure("headset", "missing");

    BOOST_TEST(called_1);
    BOOST_TEST(called_2);
}

BOOST_AUTO_TEST_CASE(multiple_fetches_does_not_affect_each_others_results)
{
    auto headset_count{0};
    fetcher_.fetch(
        product::headset,
        fetcher::unique_id(),
        fetcher::callback{
            [&headset_count](auto const & /* result */) { ++headset_count; }});

    auto fpga_count{0};
    fetcher_.fetch(
        product::fpga,
        fetcher::unique_id(),
        fetcher::callback{[&fpga_count](auto const & /* result */) { ++fpga_count; }});

    auto screen_count{0};
    fetcher_.fetch(
        product::screen,
        fetcher::unique_id(),
        fetcher::callback{
            [&screen_count](auto const & /* result */) { ++screen_count; }});

    BOOST_TEST(downloader_.download_pending("headset"));
    BOOST_TEST(downloader_.download_pending("fpga"));
    BOOST_TEST(downloader_.download_pending("screen"));

    BOOST_TEST(headset_count == 0);
    BOOST_TEST(fpga_count == 0);
    BOOST_TEST(screen_count == 0);

    downloader_.failure("headset", "missing");

    BOOST_TEST(headset_count == 1);
    BOOST_TEST(fpga_count == 0);
    BOOST_TEST(screen_count == 0);

    downloader_.success("fpga");

    BOOST_TEST(headset_count == 1);
    BOOST_TEST(fpga_count == 1);
    BOOST_TEST(screen_count == 0);

    downloader_.failure("screen", "timeout");

    BOOST_TEST(headset_count == 1);
    BOOST_TEST(fpga_count == 1);
    BOOST_TEST(screen_count == 1);
}

BOOST_AUTO_TEST_CASE(success_callback_is_only_called_once)
{
    auto fetch_count_1{0};
    fetcher_.fetch(
        product::headset,
        fetcher::unique_id(),
        fetcher::callback{
            [&fetch_count_1](auto const & /* result */) { ++fetch_count_1; }});

    BOOST_TEST(downloader_.download_pending("headset"));
    downloader_.success("headset");

    BOOST_TEST(!downloader_.download_pending("headset"));
    BOOST_TEST(fetch_count_1 == 1);

    fetcher_.fetch(
        product::headset,
        fetcher::unique_id(),
        fetcher::callback{[](auto const & /* result */) {}});

    BOOST_TEST(downloader_.download_pending("headset"));
    downloader_.success("headset");

    BOOST_TEST(fetch_count_1 == 1);
}

BOOST_AUTO_TEST_CASE(failure_callback_is_only_called_once)
{
    auto fetch_count_1{0};
    fetcher_.fetch(
        product::headset,
        fetcher::unique_id(),
        fetcher::callback{
            [&fetch_count_1](auto const & /* result */) { ++fetch_count_1; }});

    BOOST_TEST(downloader_.download_pending("headset"));
    downloader_.failure("headset", "missing");

    BOOST_TEST(!downloader_.download_pending("headset"));
    BOOST_TEST(fetch_count_1 == 1);

    fetcher_.fetch(
        product::headset,
        fetcher::unique_id(),
        fetcher::callback{[](auto const & /* result */) {}});

    BOOST_TEST(downloader_.download_pending("headset"));
    downloader_.failure("headset", "missing");

    BOOST_TEST(fetch_count_1 == 1);
}

BOOST_AUTO_TEST_SUITE_END()

}