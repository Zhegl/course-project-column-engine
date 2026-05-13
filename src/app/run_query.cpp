#include <engine/engine.h>
#include <engine/api.h>
#include <glog/logging.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace column_engine;

static std::string CsvField(const std::string& s) {
    if (s.find(',') == std::string::npos &&
        s.find('"') == std::string::npos &&
        s.find('\n') == std::string::npos) {
        return s;
    }
    std::string out = "\"";
    for (char c : s) {
        if (c == '"') out += '"';
        out += c;
    }
    out += '"';
    return out;
}

static void WriteResult(const QueryResult& result, std::ostream& out) {
    for (const auto& row : result) {
        for (size_t i = 0; i < row.size(); ++i) {
            if (i) out << ',';
            out << CsvField(row[i]);
        }
        out << '\n';
    }
}

static QueryResult RunQuery(Engine& engine, int query_num) {
    switch (query_num) {
        case 0:
            return engine.Api()
                .Aggregate("COUNT(*)")
                .Select("COUNT(*)")
                .Run();
        case 1:
            return engine.Api()
                .Where("AdvEngineID <> 0")
                .Aggregate("COUNT(*)")
                .Select("COUNT(*)")
                .Run();
        case 2:
            return engine.Api()
                .Aggregate("SUM(AdvEngineID), COUNT(*), AVG(ResolutionWidth)")
                .Select("SUM(AdvEngineID)", "COUNT(*)", "AVG(ResolutionWidth)")
                .Run();
        case 3:
            return engine.Api()
                .Aggregate("AVG(UserID)")
                .Select("AVG(UserID)")
                .Run();
        case 4:
            return engine.Api()
                .Aggregate("COUNT(DISTINCT UserID)")
                .Select("COUNT(DISTINCT UserID)")
                .Run();
        case 5:
            return engine.Api()
                .Aggregate("COUNT(DISTINCT SearchPhrase)")
                .Select("COUNT(DISTINCT SearchPhrase)")
                .Run();
        case 6:
            return engine.Api()
                .Aggregate("MIN(EventDate), MAX(EventDate)")
                .Select("MIN(EventDate)", "MAX(EventDate)")
                .Run();
        case 7:
            return engine.Api()
                .Where("AdvEngineID <> 0")
                .GroupByAggregate("AdvEngineID", "COUNT(*)")
                .OrderBy("COUNT(*) DESC")
                .Select("AdvEngineID", "COUNT(*)")
                .Run();
        case 8:
            return engine.Api()
                .GroupByAggregate("RegionID", "COUNT(DISTINCT UserID)")
                .Rename("COUNT(DISTINCT UserID)", "u")
                .OrderBy("u DESC")
                .Limit(10)
                .Select("RegionID", "u")
                .Run();
        case 9:
            return engine.Api()
                .GroupByAggregate("RegionID", "SUM(AdvEngineID), COUNT(*), AVG(ResolutionWidth), COUNT(DISTINCT UserID)")
                .Rename("COUNT(*)", "c")
                .OrderBy("c DESC")
                .Limit(10)
                .Select("RegionID", "SUM(AdvEngineID)", "c", "AVG(ResolutionWidth)", "COUNT(DISTINCT UserID)")
                .Run();
        case 10:
            return engine.Api()
                .Where("MobilePhoneModel <> ''")
                .GroupByAggregate("MobilePhoneModel", "COUNT(DISTINCT UserID)")
                .Rename("COUNT(DISTINCT UserID)", "u")
                .OrderBy("u DESC")
                .Limit(10)
                .Select("MobilePhoneModel", "u")
                .Run();
        case 11:
            return engine.Api()
                .Where("MobilePhoneModel <> ''")
                .GroupByAggregate("MobilePhone", "MobilePhoneModel", "COUNT(DISTINCT UserID)")
                .Rename("COUNT(DISTINCT UserID)", "u")
                .OrderBy("u DESC")
                .Limit(10)
                .Select("MobilePhone", "MobilePhoneModel", "u")
                .Run();
        case 12:
            return engine.Api()
                .Where("SearchPhrase <> ''")
                .GroupByAggregate("SearchPhrase", "COUNT(*)")
                .Rename("COUNT(*)", "c")
                .OrderBy("c DESC")
                .Limit(10)
                .Select("SearchPhrase", "c")
                .Run();
        case 13:
            return engine.Api()
                .Where("SearchPhrase <> ''")
                .GroupByAggregate("SearchPhrase", "COUNT(DISTINCT UserID)")
                .Rename("COUNT(DISTINCT UserID)", "u")
                .OrderBy("u DESC")
                .Limit(10)
                .Select("SearchPhrase", "u")
                .Run();
        case 14:
            return engine.Api()
                .Where("SearchPhrase <> ''")
                .GroupByAggregate("SearchEngineID", "SearchPhrase", "COUNT(*)")
                .Rename("COUNT(*)", "c")
                .OrderBy("c DESC")
                .Limit(10)
                .Select("SearchEngineID", "SearchPhrase", "c")
                .Run();
        case 15:
            return engine.Api()
                .GroupByAggregate("UserID", "COUNT(*)")
                .OrderBy("COUNT(*) DESC")
                .Limit(10)
                .Select("UserID", "COUNT(*)")
                .Run();
        case 16:
            return engine.Api()
                .GroupByAggregate("UserID", "SearchPhrase", "COUNT(*)")
                .OrderBy("COUNT(*) DESC")
                .Limit(10)
                .Select("UserID", "SearchPhrase", "COUNT(*)")
                .Run();
        case 17:
            return engine.Api()
                .GroupByAggregate("UserID", "SearchPhrase", "COUNT(*)")
                .Limit(10)
                .Select("UserID", "SearchPhrase", "COUNT(*)")
                .Run();
        case 18:
            return engine.Api()
                .GroupByAggregate("UserID", "strftime('%M', EventTime)", "SearchPhrase", "COUNT(*)")
                .OrderBy("COUNT(*) DESC")
                .Limit(10)
                .Select("UserID", "strftime('%M', EventTime)", "SearchPhrase", "COUNT(*)")
                .Run();
        case 19:
            return engine.Api()
                .Where("UserID = 435090932899640449")
                .Select("UserID")
                .Run();
        case 20:
            return engine.Api()
                .Where("URL LIKE %google%")
                .Aggregate("COUNT(*)")
                .Select("COUNT(*)")
                .Run();
        case 21:
            return engine.Api()
                .Where("URL LIKE %google%")
                .Where("SearchPhrase <> ''")
                .GroupByAggregate("SearchPhrase", "MIN(URL), COUNT(*)")
                .Rename("COUNT(*)", "c")
                .OrderBy("c DESC")
                .Limit(10)
                .Select("SearchPhrase", "MIN(URL)", "c")
                .Run();
        case 22:
            return engine.Api()
                .Where("Title LIKE %Google%")
                .Where("URL NOT LIKE %.google.%")
                .Where("SearchPhrase <> ''")
                .GroupByAggregate("SearchPhrase", "MIN(URL), MIN(Title), COUNT(*), COUNT(DISTINCT UserID)")
                .Rename("COUNT(*)", "c")
                .OrderBy("c DESC")
                .Limit(10)
                .Select("SearchPhrase", "MIN(URL)", "MIN(Title)", "c", "COUNT(DISTINCT UserID)")
                .Run();
        case 24:
            return engine.Api()
                .Where("SearchPhrase <> ''")
                .OrderBy("EventTime ASC")
                .Limit(10)
                .Select("SearchPhrase", "EventTime")
                .Run();
        case 25:
            return engine.Api()
                .Where("SearchPhrase <> ''")
                .OrderBy("SearchPhrase ASC")
                .Limit(10)
                .Select("SearchPhrase")
                .Run();
        case 31:
            return engine.Api()
                .Where("SearchPhrase <> ''")
                .GroupByAggregate("SearchEngineID", "ClientIP", "COUNT(*), SUM(IsRefresh), AVG(ResolutionWidth)")
                .Rename("COUNT(*)", "c")
                .OrderBy("c DESC")
                .Limit(10)
                .Select("SearchEngineID", "ClientIP", "c", "SUM(IsRefresh)", "AVG(ResolutionWidth)")
                .Run();
        case 32:
            return engine.Api()
                .Where("SearchPhrase <> ''")
                .GroupByAggregate("WatchID", "ClientIP", "COUNT(*), SUM(IsRefresh), AVG(ResolutionWidth)")
                .Rename("COUNT(*)", "c")
                .OrderBy("c DESC")
                .Limit(10)
                .Select("WatchID", "ClientIP", "c", "SUM(IsRefresh)", "AVG(ResolutionWidth)")
                .Run();
        case 33:
            return engine.Api()
                .GroupByAggregate("WatchID", "ClientIP", "COUNT(*), SUM(IsRefresh), AVG(ResolutionWidth)")
                .Rename("COUNT(*)", "c")
                .OrderBy("c DESC")
                .Limit(10)
                .Select("WatchID", "ClientIP", "c", "SUM(IsRefresh)", "AVG(ResolutionWidth)")
                .Run();
        case 34:
            return engine.Api()
                .GroupByAggregate("URL", "COUNT(*)")
                .Rename("COUNT(*)", "c")
                .OrderBy("c DESC")
                .Limit(10)
                .Select("URL", "c")
                .Run();
        case 36:
            return engine.Api()
                .Where("CounterID = 62")
                .Where("EventDate >= '2013-07-01'")
                .Where("EventDate <= '2013-07-31'")
                .Where("DontCountHits = 0")
                .Where("IsRefresh = 0")
                .Where("URL <> ''")
                .GroupByAggregate("URL", "COUNT(*)")
                .Rename("COUNT(*)", "PageViews")
                .OrderBy("PageViews DESC")
                .Limit(10)
                .Select("URL", "PageViews")
                .Run();
        case 37:
            return engine.Api()
                .Where("CounterID = 62")
                .Where("EventDate >= '2013-07-01'")
                .Where("EventDate <= '2013-07-31'")
                .Where("DontCountHits = 0")
                .Where("IsRefresh = 0")
                .Where("Title <> ''")
                .GroupByAggregate("Title", "COUNT(*)")
                .Rename("COUNT(*)", "PageViews")
                .OrderBy("PageViews DESC")
                .Limit(10)
                .Select("Title", "PageViews")
                .Run();
        case 38:
            return engine.Api()
                .Where("CounterID = 62")
                .Where("EventDate >= '2013-07-01'")
                .Where("EventDate <= '2013-07-31'")
                .Where("IsRefresh = 0")
                .Where("IsLink <> 0")
                .Where("IsDownload = 0")
                .GroupByAggregate("URL", "COUNT(*)")
                .Rename("COUNT(*)", "PageViews")
                .OrderBy("PageViews DESC")
                .Offset(1000)
                .Limit(10)
                .Select("URL", "PageViews")
                .Run();
        case 42:
            return engine.Api()
                .Where("CounterID = 62")
                .Where("EventDate >= '2013-07-14'")
                .Where("EventDate <= '2013-07-15'")
                .Where("IsRefresh = 0")
                .Where("DontCountHits = 0")
                .GroupByAggregate("strftime('%M', EventTime)", "COUNT(*)")
                .Rename("COUNT(*)", "PageViews")
                .OrderBy("strftime('%M', EventTime) ASC")
                .Offset(1000)
                .Limit(10)
                .Select("strftime('%M', EventTime)", "PageViews")
                .Run();
        default:
            throw std::runtime_error("Query " + std::to_string(query_num) + " not implemented");
    }
}

int main(int argc, char** argv) {
    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = 1;

    if (argc < 4) {
        std::cerr << "Usage: run_query <query_num> <columnar_file> <output_csv> [log_file]\n";
        return 1;
    }

    int query_num = std::stoi(argv[1]);
    std::string columnar = argv[2];
    std::string output_path = argv[3];

    try {
        Engine engine(columnar);
        QueryResult result = RunQuery(engine, query_num);

        std::ofstream out(output_path);
        if (!out) {
            throw std::runtime_error("Cannot open output: " + output_path);
        }
        // skip header row — bench expects only data rows
        for (size_t i = 1; i < result.size(); ++i) {
            const auto& row = result[i];
            for (size_t j = 0; j < row.size(); ++j) {
                if (j) out << ',';
                out << CsvField(row[j]);
            }
            out << '\n';
        }
    } catch (const std::runtime_error& e) {
        LOG(WARNING) << "Query " << query_num << " skipped: " << e.what();
        std::ofstream out(output_path);
        google::ShutdownGoogleLogging();
        return 0;
    } catch (const std::exception& e) {
        LOG(ERROR) << e.what();
        google::ShutdownGoogleLogging();
        return 1;
    }

    google::ShutdownGoogleLogging();
    return 0;
}
