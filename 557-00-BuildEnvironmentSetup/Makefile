########################################################################
####################### Makefile Template ##############################
########################################################################

APPNAME = Vulkan00
DEBUG = -g
VERBOSE = -v

# For defining platform specific flags
PLATFORM = $(shell uname -s)
# Handle special case for Windows 10 and 11 uname difference
ifeq ($(PLATFORM), Windows_NT)
PLATFORM = Windows
else ifeq ($(shell echo $(PLATFORM) | cut -c1-7), MSYS_NT)
PLATFORM = Windows
endif

# Windiows definitions
ifeq ($(PLATFORM), Windows)
	APP_NAME = $(APPNAME).exe
	GLM_PATH = $(VULKAN_SDK)/include
	GLFW_PATH = $(VULKAN_SDK)/glfw-3.4.bin.WIN64/include
	GLFW_LIB_PATH = $(VULKAN_SDK)/glfw-3.4.bin.WIN64/lib-mingw-w64
	LDFLAGS = -L$(VULKAN_SDK)/Lib -L$(GLFW_LIB_PATH) -lvulkan-1 -lglfw3 -lgdi32

# MacOS definitions
else ifeq ($(PLATFORM), Darwin)
	APP_NAME = $(APPNAME)
	GLM_PATH = $(VULKAN_SDK)/include
	GLFW_PATH = $(VULKAN_SDK)/glfw-3.4.bin.MACOS/include
	GLFW_LIB_PATH = $(VULKAN_SDK)/glfw-3.4.bin.MACOS/lib-arm64
	LDFLAGS = -L$(VULKAN_SDK)/lib -L$(GLFW_LIB_PATH) -lvulkan -lglfw3 -framework Cocoa -framework IOKit

# Linux definitions
else ifeq ($(PLATFORM), Linux)
	APP_NAME = $(APPNAME)
	GLM_PATH = /usr/include/glm-master
	GLFW_PATH = /usr/include/GLFW
	GLFW_LIB_PATH = /usr/lib/x86_64-linux-gnu
	LDFLAGS = -L$(VULKAN_SDK)/lib -L$(GLFW_LIB_PATH) -lvulkan -lglfw
endif

CFLAGS = -std=c++17 $(DEBUG) -I. -I$(VULKAN_SDK)/include -I$(GLM_PATH) -I$(GLFW_PATH)
include LOCAL_SRCS

$(APP_NAME):
	g++ $(CFLAGS) -o $(APP_NAME) $(CPPSRCS) $(LDFLAGS)

run:
	./$(APP_NAME)

clean:
	rm -f $(APP_NAME)

test:
	@echo $(PLATFORM)
