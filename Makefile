# 编译器设置
CC = gcc
CXX = g++
CFLAGS = -Wall -Wextra -std=c99 -D_GNU_SOURCE
CXXFLAGS = -Wall -Wextra -std=c++11
LDFLAGS = -lwiringPi -lpthread -lcjson -lm `pkg-config --libs opencv4`
INCLUDES = -Icomponents -Icombo -Iweb `pkg-config --cflags opencv4`

# 源文件
C_SRCS = main.c \
         components/botton.c components/clock.c components/beep.c components/rgb.c components/DHT.c components/usonic.c components/servo.c components/control.c \
         combo/alarm_clock.c combo/stopwatch.c combo/rgb_control.c combo/temp_display.c \
         web/http_server.c web/api_handlers.c

CXX_SRCS = components/camera.cpp

C_OBJS = $(addprefix target/,$(notdir $(C_SRCS:.c=.o)))
CXX_OBJS = $(addprefix target/,$(notdir $(CXX_SRCS:.cpp=.o)))
OBJS = $(C_OBJS) $(CXX_OBJS)
TARGET = main_app

# 默认目标
all: target_dir $(TARGET)

# 创建目标目录
target_dir:
	@mkdir -p target

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS)

# C文件编译规则
target/%.o: components/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

target/%.o: combo/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

target/%.o: web/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

target/main.o: main.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# C++文件编译规则
target/%.o: components/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# 清理
clean:
	rm -f $(TARGET) target/*.o
	rmdir target 2>/dev/null || true

# 重新编译
rebuild: clean all

.PHONY: all clean rebuild target_dir
