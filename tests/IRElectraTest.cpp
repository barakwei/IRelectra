#include <memory>
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <memory>

#include "IRremote.h"
#include "IRElectra.h"

using std::string;
using std::size_t;
using std::shared_ptr;
using std::unique_ptr;

struct ElectraConfig
{
public:
    bool power;
    IRElectraMode mode;
    IRElectraFan fan;
    uint32_t temperature;
    bool swing;
    bool sleep;

    bool operator==(const ElectraConfig &other) const
    {
        return (power == other.power &&
            mode == other.mode &&
            fan == other.fan &&
            temperature == other.temperature &&
            swing == other.swing &&
            sleep == other.sleep);
    }
};

namespace std {

    template <>
    struct hash<ElectraConfig>
    {
        std::size_t operator()(const ElectraConfig& k) const
        {
            using std::size_t;
            using std::hash;
            using std::string;

            return (hash<bool>()(k.power) ^
                (hash<uint32_t>()(k.mode) >> 1) ^
                (hash<uint32_t>()(k.fan) >> 9) ^
                (hash<int>()(k.temperature) >> 17) ^
                (hash<bool>()(k.swing) >> 25) ^
                (hash<bool>()(k.sleep) >> 26));
        }
    };

    inline string to_string(bool val)
    {
        if (val)
            return "T";
        else
            return "F";
    }

    inline string to_string(ElectraConfig val)
    {
        return "p: " + std::to_string(val.power) +
            "; m: " + std::to_string(val.mode) +
            "; f: " + std::to_string(val.fan) +
            "; t: " + std::to_string(val.temperature) +
            "; sw: " + std::to_string(val.swing) +
            "; sl: " + std::to_string(val.sleep);
    }
}

bool areVectorsEqual(const std::vector<unsigned int>& a, const std::vector<unsigned int>& b)
{
    for (unsigned int i = 0; i < std::min(a.size(), b.size()); ++i)
    {
        if (a[i] != b[i])
        {
            std::cout << i+1 << ": " << a[i] << "!=" << b[i] << std::endl;
        }
    }
    return a == b;
}

typedef std::vector<std::pair<ElectraConfig, std::vector<unsigned int>>> RemoteTestVector;
typedef std::vector<std::pair<std::shared_ptr<ElectraRemote>, RemoteTestVector>> RemoteTestItems;

struct RemoteSignalTestResult
{
public:
    bool success;
    std::string text;
};

class Printer
{
private:
    const int indentWidth = 2;
    int _indent = 0;

public:
    void addIndent()
    {
        ++_indent;
    }

    void removeIndent()
    {
        --_indent;
    }

    void print(string& text)
    {
        std::cout << string(indentWidth*_indent, ' ') << text;
    }
};

class TestTracker
{
private:
    std::vector<std::string> _successes;
    std::vector<std::string> _failures;

public:
    void addSuccess(std::string& text)
    {
        _successes.emplace_back(text);
        std::cout << text << std::endl;
    }

    void addFailure(std::string& text)
    {
        _failures.emplace_back(text);
        std::cout << text << std::endl;
    }

    const std::vector<std::string> successes() { return _successes; }
    const std::vector<std::string> failures() { return _failures; }
};

struct ExpectationResult
{
    bool asExpected;
    std::string text;
};

class Expector
{
private:
    std::shared_ptr<Printer> _printer;
    uint32_t _numFailures;
public:
    Expector(std::shared_ptr<Printer> printer) : _printer(printer), _numFailures(0)
    {}

    uint32_t numFailedExpectations() { return _numFailures; }

    template<typename T>
    ExpectationResult _equals(const T& lhs, const T& rhs)
    {
        if (lhs == rhs) {
            return { true };
        }
        return (false, std::to_string(lhs) + " is not equal to " + std::to_string(rhs));
    }

    template <>
    ExpectationResult _equals<std::vector<unsigned int>>(const std::vector<unsigned int>& lhs, const std::vector<unsigned int>& rhs)
    {
        bool failed = false;
        std::string failureText;
        if (lhs.size() != rhs.size()) {
            failed = true;
            failureText.append("vecor size " + std::to_string(lhs.size()) + " != vector size " + std::to_string(rhs.size()) + "\n");
        }
        for (unsigned int i = 0; i < std::min(lhs.size(), rhs.size()); ++i)
        {
            if (lhs[i] != rhs[i])
            {
                failureText.append(std::to_string(i + 1) + ": " + std::to_string(lhs[i]) + "!=" + std::to_string(rhs[i]) + "\n");
                failed = true;
            }
        }
        if (failed)
            return { false, failureText };
        return {true};
    }

    template<typename T>
    void equals(const T& lhs, const T& rhs)
    {
        auto result = _equals(lhs, rhs);
        if (!result.asExpected)
        {
            _printer->print(result.text);
            _numFailures++;
        }
    }
};

class RemoteTester
{
private:
    RemoteTestItems _itemsToTest;
    std::shared_ptr<Printer> _printer;
    std::shared_ptr<TestTracker> _tracker;
    std::unique_ptr<Expector> _expect;

    void testRemoteSignal(const std::shared_ptr<ElectraRemote> remote, const ElectraConfig& config, std::vector<unsigned int>& gt)
    {
        using std::string;
        _expect = std::make_unique<Expector>(_printer);
        std::string testName = string(remote->name()) + ": " + std::to_string(config);
        _tracker->addSuccess("[RUNNING   ] " + testName);
        _testRemoteSignal(remote, config, gt);

        if (_expect->numFailedExpectations())
        {
           _tracker->addFailure("[   FAILURE] " + testName);
        }
        else
        {
            _tracker->addSuccess("[   SUCCESS] " + testName);
        }
    }

    void _testRemoteSignal(const std::shared_ptr<ElectraRemote> remote, const ElectraConfig& config, std::vector<unsigned int>& gt)
    {
        auto data = remote->fullPacket(true, IRElectraModeCool, IRElectraFanLow, 24, false, false);
        _expect->equals(gt, data.data());
    }

    void testRemote(const std::shared_ptr<ElectraRemote> remote, RemoteTestVector& vector)
    {
        for (auto& test : vector)
        {
            testRemoteSignal(remote, test.first, test.second);
        }
    }
public:
    RemoteTester(RemoteTestItems& itemsToTest) : _itemsToTest(itemsToTest), 
        _tracker(std::make_shared<TestTracker>()), _printer(std::make_shared<Printer>())
    {}

    void testAllRemotes(RemoteTestItems& allRemoteTests)
    {
        for (auto& singleRemoteTests : allRemoteTests)
        {
            testRemote(singleRemoteTests.first, singleRemoteTests.second);
        }
        std::cout << "Done running tests" << std::endl << std::endl;

        auto failures = _tracker->failures();
        if (failures.size())
        {
            for (auto& failText : failures)
            {
                std::cout << failText << std::endl;
            }
            std::cout << failures.size() << " test(s) failed" << std::endl;
        }
        else
        {
            std::cout << "All is good" << std::endl;
        }
    }
};

RemoteTestVector orangeTestVector = {
    { { true, IRElectraModeCool, IRElectraFanLow, 24, false, false }, { 2976, 3968, 1984, 992, 992, 1984, 1984, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 1984, 1984, 992, 992, 1984, 1984, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 1984, 1984, 992, 2976, 3968, 1984, 992, 992, 1984, 1984, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 1984, 1984, 992, 992, 1984, 1984, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 1984, 1984, 992, 2976, 3968, 1984, 992, 992, 1984, 1984, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 1984, 1984, 992, 992, 1984, 1984, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 992, 1984, 1984, 992, 3968 } }
};

RemoteTestVector greenTestVector = {
    { { true, IRElectraModeCool, IRElectraFanLow, 24, false, false }, { 9000, 4500, 560, 1680, 560, 1680, 560, 560, 560, 560, 560, 560, 560, 560, 560, 1680, 560, 1680, 560, 1680, 560, 1680, 560, 1680, 560, 560, 560, 560, 560, 560, 560, 560, 560, 1680, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 1680, 560, 1680, 560, 1680, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 1680, 560, 1680, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 1680, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 1680, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 1680, 560, 560, 560, 1680, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 1680, 560, 1680, 560, 1680, 560, 1680, 560, 560, 560, 560, 560, 1680, 560, 1680, 560 } }
};

int main()
{
    using std::make_shared;
    auto orange = OrangeElectraRemote();
    RemoteTestItems testItems = {
        { make_shared<OrangeElectraRemote>(), orangeTestVector},
        { make_shared<GreenElectraRemote>(), greenTestVector },
    };
    RemoteTester tester(testItems);
    tester.testAllRemotes(testItems);
}