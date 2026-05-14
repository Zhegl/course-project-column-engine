#include <engine/engine.h>
#include <engine/api.h>
#include <glog/logging.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

using column_engine::Engine;
using column_engine::QueryResult;

static std::string CsvField(const std::string& s) {
    if (s.find(',') == std::string::npos &&
        s.find('"') == std::string::npos &&
        s.find('\n') == std::string::npos) {
        return s;
    }
    std::string out = "\"";
    for (char c : s) {
        if (c == '"') {
            out += '"';
        }
        out += c;
    }
    out += '"';
    return out;
}

static void WriteResult(const QueryResult& result, std::ostream& out) {
    for (const auto& row : result) {
        for (size_t i = 0; i < row.size(); ++i) {
            if (i) {
                out << ',';
            }
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
                .Add("strftime('%M', EventTime)")
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
        case 26:
            return engine.Api()
                .Where("SearchPhrase <> ''")
                .OrderBy("SearchPhrase ASC")
                .Limit(10)
                .Select("SearchPhrase")
                .Run();
        case 28:
            return engine.Api()
                .Where("URL <> ''")
                .Add("length(URL)")
                .GroupByAggregate("CounterID", "AVG(length(URL)), COUNT(*), MIN(URL)")
                .Rename("AVG(length(URL))", "l")
                .Rename("COUNT(*)", "c")
                .Where("c > 100000")
                .OrderBy("l DESC")
                .Limit(25)
                .Select("CounterID", "l", "c", "MIN(URL)")
                .Run();
        case 29:
            return engine.Api()
                .Add("ResolutionWidth + 0")
                .Add("ResolutionWidth + 1")
                .Add("ResolutionWidth + 2")
                .Add("ResolutionWidth + 3")
                .Add("ResolutionWidth + 4")
                .Add("ResolutionWidth + 5")
                .Add("ResolutionWidth + 6")
                .Add("ResolutionWidth + 7")
                .Add("ResolutionWidth + 8")
                .Add("ResolutionWidth + 9")
                .Add("ResolutionWidth + 10")
                .Add("ResolutionWidth + 11")
                .Add("ResolutionWidth + 12")
                .Add("ResolutionWidth + 13")
                .Add("ResolutionWidth + 14")
                .Add("ResolutionWidth + 15")
                .Add("ResolutionWidth + 16")
                .Add("ResolutionWidth + 17")
                .Add("ResolutionWidth + 18")
                .Add("ResolutionWidth + 19")
                .Add("ResolutionWidth + 20")
                .Add("ResolutionWidth + 21")
                .Add("ResolutionWidth + 22")
                .Add("ResolutionWidth + 23")
                .Add("ResolutionWidth + 24")
                .Add("ResolutionWidth + 25")
                .Add("ResolutionWidth + 26")
                .Add("ResolutionWidth + 27")
                .Add("ResolutionWidth + 28")
                .Add("ResolutionWidth + 29")
                .Add("ResolutionWidth + 30")
                .Add("ResolutionWidth + 31")
                .Add("ResolutionWidth + 32")
                .Add("ResolutionWidth + 33")
                .Add("ResolutionWidth + 34")
                .Add("ResolutionWidth + 35")
                .Add("ResolutionWidth + 36")
                .Add("ResolutionWidth + 37")
                .Add("ResolutionWidth + 38")
                .Add("ResolutionWidth + 39")
                .Add("ResolutionWidth + 40")
                .Add("ResolutionWidth + 41")
                .Add("ResolutionWidth + 42")
                .Add("ResolutionWidth + 43")
                .Add("ResolutionWidth + 44")
                .Add("ResolutionWidth + 45")
                .Add("ResolutionWidth + 46")
                .Add("ResolutionWidth + 47")
                .Add("ResolutionWidth + 48")
                .Add("ResolutionWidth + 49")
                .Add("ResolutionWidth + 50")
                .Add("ResolutionWidth + 51")
                .Add("ResolutionWidth + 52")
                .Add("ResolutionWidth + 53")
                .Add("ResolutionWidth + 54")
                .Add("ResolutionWidth + 55")
                .Add("ResolutionWidth + 56")
                .Add("ResolutionWidth + 57")
                .Add("ResolutionWidth + 58")
                .Add("ResolutionWidth + 59")
                .Add("ResolutionWidth + 60")
                .Add("ResolutionWidth + 61")
                .Add("ResolutionWidth + 62")
                .Add("ResolutionWidth + 63")
                .Add("ResolutionWidth + 64")
                .Add("ResolutionWidth + 65")
                .Add("ResolutionWidth + 66")
                .Add("ResolutionWidth + 67")
                .Add("ResolutionWidth + 68")
                .Add("ResolutionWidth + 69")
                .Add("ResolutionWidth + 70")
                .Add("ResolutionWidth + 71")
                .Add("ResolutionWidth + 72")
                .Add("ResolutionWidth + 73")
                .Add("ResolutionWidth + 74")
                .Add("ResolutionWidth + 75")
                .Add("ResolutionWidth + 76")
                .Add("ResolutionWidth + 77")
                .Add("ResolutionWidth + 78")
                .Add("ResolutionWidth + 79")
                .Add("ResolutionWidth + 80")
                .Add("ResolutionWidth + 81")
                .Add("ResolutionWidth + 82")
                .Add("ResolutionWidth + 83")
                .Add("ResolutionWidth + 84")
                .Add("ResolutionWidth + 85")
                .Add("ResolutionWidth + 86")
                .Add("ResolutionWidth + 87")
                .Add("ResolutionWidth + 88")
                .Add("ResolutionWidth + 89")
                .Aggregate("SUM(ResolutionWidth + 0), SUM(ResolutionWidth + 1), SUM(ResolutionWidth + 2), SUM(ResolutionWidth + 3), SUM(ResolutionWidth + 4), SUM(ResolutionWidth + 5), SUM(ResolutionWidth + 6), SUM(ResolutionWidth + 7), SUM(ResolutionWidth + 8), SUM(ResolutionWidth + 9), SUM(ResolutionWidth + 10), SUM(ResolutionWidth + 11), SUM(ResolutionWidth + 12), SUM(ResolutionWidth + 13), SUM(ResolutionWidth + 14), SUM(ResolutionWidth + 15), SUM(ResolutionWidth + 16), SUM(ResolutionWidth + 17), SUM(ResolutionWidth + 18), SUM(ResolutionWidth + 19), SUM(ResolutionWidth + 20), SUM(ResolutionWidth + 21), SUM(ResolutionWidth + 22), SUM(ResolutionWidth + 23), SUM(ResolutionWidth + 24), SUM(ResolutionWidth + 25), SUM(ResolutionWidth + 26), SUM(ResolutionWidth + 27), SUM(ResolutionWidth + 28), SUM(ResolutionWidth + 29), SUM(ResolutionWidth + 30), SUM(ResolutionWidth + 31), SUM(ResolutionWidth + 32), SUM(ResolutionWidth + 33), SUM(ResolutionWidth + 34), SUM(ResolutionWidth + 35), SUM(ResolutionWidth + 36), SUM(ResolutionWidth + 37), SUM(ResolutionWidth + 38), SUM(ResolutionWidth + 39), SUM(ResolutionWidth + 40), SUM(ResolutionWidth + 41), SUM(ResolutionWidth + 42), SUM(ResolutionWidth + 43), SUM(ResolutionWidth + 44), SUM(ResolutionWidth + 45), SUM(ResolutionWidth + 46), SUM(ResolutionWidth + 47), SUM(ResolutionWidth + 48), SUM(ResolutionWidth + 49), SUM(ResolutionWidth + 50), SUM(ResolutionWidth + 51), SUM(ResolutionWidth + 52), SUM(ResolutionWidth + 53), SUM(ResolutionWidth + 54), SUM(ResolutionWidth + 55), SUM(ResolutionWidth + 56), SUM(ResolutionWidth + 57), SUM(ResolutionWidth + 58), SUM(ResolutionWidth + 59), SUM(ResolutionWidth + 60), SUM(ResolutionWidth + 61), SUM(ResolutionWidth + 62), SUM(ResolutionWidth + 63), SUM(ResolutionWidth + 64), SUM(ResolutionWidth + 65), SUM(ResolutionWidth + 66), SUM(ResolutionWidth + 67), SUM(ResolutionWidth + 68), SUM(ResolutionWidth + 69), SUM(ResolutionWidth + 70), SUM(ResolutionWidth + 71), SUM(ResolutionWidth + 72), SUM(ResolutionWidth + 73), SUM(ResolutionWidth + 74), SUM(ResolutionWidth + 75), SUM(ResolutionWidth + 76), SUM(ResolutionWidth + 77), SUM(ResolutionWidth + 78), SUM(ResolutionWidth + 79), SUM(ResolutionWidth + 80), SUM(ResolutionWidth + 81), SUM(ResolutionWidth + 82), SUM(ResolutionWidth + 83), SUM(ResolutionWidth + 84), SUM(ResolutionWidth + 85), SUM(ResolutionWidth + 86), SUM(ResolutionWidth + 87), SUM(ResolutionWidth + 88), SUM(ResolutionWidth + 89)")
                .Run();
        case 35:
            return engine.Api()
                .Add("ClientIP - 1")
                .Add("ClientIP - 2")
                .Add("ClientIP - 3")
                .GroupByAggregate("ClientIP", "ClientIP - 1", "ClientIP - 2", "ClientIP - 3", "COUNT(*)")
                .Rename("COUNT(*)", "c")
                .OrderBy("c DESC")
                .Limit(10)
                .Select("ClientIP", "ClientIP - 1", "ClientIP - 2", "ClientIP - 3", "c")
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
        case 40:
            return engine.Api()
                .Where("CounterID = 62")
                .Where("EventDate >= '2013-07-01'")
                .Where("EventDate <= '2013-07-31'")
                .Where("IsRefresh = 0")
                .Where("TraficSourceID IN (-1, 6)")
                .Where("RefererHash = 3594120000172545465")
                .GroupByAggregate("URLHash", "EventDate", "COUNT(*)")
                .Rename("COUNT(*)", "PageViews")
                .OrderBy("PageViews DESC")
                .Offset(100)
                .Limit(10)
                .Select("URLHash", "EventDate", "PageViews")
                .Run();
        case 41:
            return engine.Api()
                .Where("CounterID = 62")
                .Where("EventDate >= '2013-07-01'")
                .Where("EventDate <= '2013-07-31'")
                .Where("IsRefresh = 0")
                .Where("TraficSourceID IN (-1, 6)")
                .Where("RefererHash = 3594120000172545465")
                .GroupByAggregate("URLHash", "EventDate", "COUNT(*)")
                .Rename("COUNT(*)", "PageViews")
                .OrderBy("PageViews DESC")
                .Offset(100)
                .Limit(10)
                .Select("URLHash", "EventDate", "PageViews")
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
                if (j) {
                    out << ',';
                }
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
