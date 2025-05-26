########################################################################
####################### Makefile Template ##############################
########################################################################

DEBUG = -g
VERBOSE = -v
UNAME = $(shell uname -s)
APPNAME = Vulkan01

# Windiows definitions
ifeq ($(UNAME), Windows_NT)
	APP_NAME = $(APPNAME).exe
	GLM_PATH = $(VULKAN_SDK)/include
	GLFW_PATH = $(VULKAN_SDK)/glfw-3.3.9.bin.WIN64/include
	GLFW_LIB_PATH = $(VULKAN_SDK)/glfw-3.3.9.bin.WIN64/lib-mingw-w64
	LDFLAGS = -L$(VULKAN_SDK)/Lib -L$(GLFW_LIB_PATH) -lvulkan-1 -lglfw3 -lgdi32
endif

# MacOS definitions
ifeq ($(UNAME), Darwin)
	APP_NAME = $(APPNAME)
	GLM_PATH = $(VULKAN_SDK)/include
	GLFW_PATH = $(VULKAN_SDK)/glfw-3.3.9.bin.MACOS/include
	GLFW_LIB_PATH = $(VULKAN_SDK)/glfw-3.3.9.bin.MACOS/lib-x86_64
	LDFLAGS = -L$(VULKAN_SDK)/lib -L$(GLFW_LIB_PATH) -lvulkan -lglfw
endif

CFLAGS = -std=c++17 $(DEBUG) -I. -I$(VULKAN_SDK)/include -I$(GLM_PATH) -I$(GLFW_PATH)
include LOCAL_SRCS

$(APP_NAME): *.cpp
	g++ $(CFLAGS) -o $(APP_NAME) $(CPPSRCS) $(LDFLAGS)

run: $(APP_NAME)
	.\$(APP_NAME)

clean:
	rm -f $(APP_NAME)

test:
	@echo $(UNAME)