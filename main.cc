#include <inotify-cpp/NotifierBuilder.h>

#include <filesystem>

#include <iostream>
#include <thread>
#include <chrono>

#include <ctime>
#include <iomanip>
#include <string>
#include <sstream>
#include <fstream>

using namespace inotify;

time_t steady_clock_to_time_t(
    const std::chrono::steady_clock::time_point &steady_tp)
{
  // 1. 获取当前系统时间点
  auto sys_now = std::chrono::system_clock::now();

  // 2. 获取当前稳定时钟时间点
  auto steady_now = std::chrono::steady_clock::now();

  // 3. 计算稳定时钟时间点与当前稳定时钟的偏移
  auto offset = steady_now - steady_tp;

  // 4. 将偏移量应用到系统时间点
  auto sys_tp = sys_now - offset;

  // 5. 转换为 time_t
  return std::chrono::system_clock::to_time_t(sys_tp);
}

std::string getEpoch(const std::chrono::steady_clock::time_point &time)
{

  // 将时间转换为可读格式
  std::time_t t = steady_clock_to_time_t(time);
  std::tm tm = *std::localtime(&t);

  std::ostringstream oss{};
  oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
  return oss.str();
}

std::string extractGrpcUrl(const std::string &configFile)
{
  std::ifstream file(configFile);
  std::string line;
  std::string grpcUrl;

  if (!file.is_open())
  {
    std::cerr << "Unable to open file: " << configFile << std::endl;
    return "";
  }

  while (std::getline(file, line))
  {
    // 如果是 # 打头就跳过
    if (line[0] == '#' || line[0] == ';')
    {
      continue;
    }
    // 查找包含 "grpc-url" 的行
    size_t pos = line.find("grpc-url: ");
    if (pos != std::string::npos)
    {
      // 提取该行的地址部分
      std::stringstream ss(line.substr(pos + 10)); // 从 "grpc-url:" 后面开始提取
      std::getline(ss, grpcUrl);
      break;
    }
  }

  file.close();
  return grpcUrl;
}

int main(int argc, char **argv)
{
  if (argc <= 1)
  {
    std::cout << "Usage: ./inotify_example /path/to/dir" << std::endl;
    exit(0);
  }

  // Parse the directory to watch
  std::filesystem::path path(argv[1]);

  // Set the event handler which will be used to process particular events
  auto handleNotification = [&](Notification notification)
  {
    std::string addr = extractGrpcUrl(path.string());
    if (!addr.empty())
    {
      std::cout << "Event " << notification.event << " on " << notification.path << " at "
                << getEpoch(notification.time) << " was triggered: "
                << addr
                << std::endl;
    }
  };

  // Set the events to be notified for
  auto events = {Event::modify};

  // The notifier is configured to watch the parsed path for the defined events. Particular files
  // or paths can be ignored(once).
  auto notifier = BuildNotifier()
                      .watchPathRecursively(path)
                      .ignoreFileOnce("fileIgnoredOnce")
                      .ignoreFile("fileIgnored")
                      .onEvents(events, handleNotification);

  // The event loop is started in a separate thread context.
  std::thread thread([&]()
                     { notifier.run(); });

  // Terminate the event loop after 60 seconds
  std::this_thread::sleep_for(std::chrono::seconds(60));
  notifier.stop();
  thread.join();
  return 0;
}
