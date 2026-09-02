#include <vengine/core/Config.hpp>
#include <vengine/core/Error.hpp>
#include <vengine/core/Logging.hpp>

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

using namespace vengine::core;

namespace {
std::string tmp_path(const char* name) {
    auto p = std::filesystem::temp_directory_path() / name;
    return p.string();
}
} // namespace

TEST(Config, SetGetTyped) {
    Config c;
    c.set("width", i64{1280});
    c.set("fullscreen", true);
    c.set("title", std::string{"My Game"});
    c.set("volume", 0.75);
    EXPECT_EQ(c.get_int("width"), 1280);
    EXPECT_TRUE(c.get_bool("fullscreen"));
    EXPECT_EQ(c.get_string("title"), "My Game");
    EXPECT_NEAR(c.get_float("volume"), 0.75, 1e-9);
    // fallbacks
    EXPECT_EQ(c.get_int("missing", 42), 42);
    EXPECT_FALSE(c.get_bool("missing"));
}

TEST(Config, AtomicSaveLoadRoundTrip) {
    const auto path = tmp_path("vengine_config_test.ini");
    std::remove(path.c_str());

    Config c;
    c.set("width", i64{1920});
    c.set("height", i64{1080});
    c.set("name", std::string{"Round Trip"});
    c.set("debug", true);
    c.set("gamma", 2.2);

    auto res = c.save(path);
    ASSERT_TRUE(res.ok()) << res.error().format();

    // A .tmp file must NOT be left behind after a successful save.
    EXPECT_FALSE(std::filesystem::exists(std::string(path) + ".tmp"));

    Config loaded;
    auto lres = loaded.load(path);
    ASSERT_TRUE(lres.ok()) << lres.error().format();
    EXPECT_EQ(loaded.get_int("width"), 1920);
    EXPECT_EQ(loaded.get_int("height"), 1080);
    EXPECT_EQ(loaded.get_string("name"), "Round Trip");
    EXPECT_TRUE(loaded.get_bool("debug"));
    EXPECT_NEAR(loaded.get_float("gamma"), 2.2, 1e-9);

    std::remove(path.c_str());
}

TEST(Config, LoadMissingFileIsOk) {
    Config c;
    auto res = c.load("/no/such/path/vengine_does_not_exist.ini");
    EXPECT_TRUE(res.ok());
}

TEST(Config, MalformedLineIsSkippedNotFatal) {
    const auto path = tmp_path("vengine_config_malformed.ini");
    {
        std::ofstream o(path);
        o << "good = 1\n"
          << "this line has no equals\n"
          << "  # comment\n"
          << "other = \"hello world\"\n";
    }
    Config c;
    auto res = c.load(path);
    EXPECT_TRUE(res.ok());
    EXPECT_EQ(c.get_int("good"), 1);
    EXPECT_EQ(c.get_string("other"), "hello world");
    EXPECT_FALSE(c.has("this line has no equals"));
    std::remove(path.c_str());
}

TEST(Error, FormatAndPropagation) {
    auto e = Error::from(ErrorCode::AssetNotFound, "player.png", RecoveryHint::ReimportAsset);
    EXPECT_EQ(e.code, ErrorCode::AssetNotFound);
    EXPECT_FALSE(e.ok());
    const auto s = e.format();
    EXPECT_NE(s.find("AssetNotFound"), std::string::npos);
    EXPECT_NE(s.find("player.png"), std::string::npos);
}

TEST(Result, OkAndFail) {
    Result<int> ok = Result<int>::Ok(7);
    ASSERT_TRUE(ok.ok());
    EXPECT_EQ(ok.value(), 7);

    Result<int> fail = Result<int>::Fail(ErrorCode::OutOfMemory, "boom");
    ASSERT_FALSE(fail.ok());
    EXPECT_EQ(fail.error().code, ErrorCode::OutOfMemory);
}

TEST(Logging, MinLevelFilters) {
    Logger::instance().set_min_level(LogLevel::Warn);
    // These must not crash; lower levels are dropped.
    VENGINE_LOG_DEBUG("Test", "dropped");
    VENGINE_LOG_WARN("Test", "kept %d", 42);
    Logger::instance().set_min_level(LogLevel::Info);
}
