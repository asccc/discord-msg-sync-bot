TARGET   = discord-news-sync
BOT_USER = peakstreams
CXX      = g++
CXXFLAGS = -std=c++17 -O3 -Wall -DNDEBUG
LDFLAGS  = $(shell pkg-config --libs dpp) -lmariadb
INCLUDE  = $(shell pkg-config --cflags dpp)

PREFIX      = /usr/local
BINDIR      = $(PREFIX)/bin
SERVICEDIR  = /etc/systemd/system
SERVICE_NAME = discord-news-sync.service

SRCS = $(wildcard src/*.cpp)
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	@echo "Linking $@..."
	$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.cpp
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

install: all
	@echo "Installing binary to $(BINDIR)..."
	install -m 755 $(TARGET) $(BINDIR)/$(TARGET)
	chown $(BOT_USER):$(BOT_USER) $(BINDIR)/$(TARGET)

	@echo "Creating systemd service for user $(BOT_USER)..."
	@printf "[Unit]\n\
Description=Discord News Sync Bot\n\
After=network.target mariadb.service\n\n\
[Service]\n\
Type=simple\n\
User=$(BOT_USER)\n\
Group=$(BOT_USER)\n\
EnvironmentFile=/etc/discord-news-sync.env\n\
WorkingDirectory=$(BINDIR)\n\
ExecStart=$(BINDIR)/$(TARGET)\n\
Restart=always\n\
RestartSec=5\n\
StandardOutput=journal\n\
StandardError=journal\n\n\
[Install]\n\
WantedBy=multi-user.target\n" > $(SERVICE_NAME)

	mv $(SERVICE_NAME) $(SERVICEDIR)/$(SERVICE_NAME)
	systemctl daemon-reload
	
	@echo "------------------------------------------------------"
	@echo "Installation finished. Commands:"
	@echo "Start:   sudo systemctl start $(SERVICE_NAME)"
	@echo "Status:  sudo systemctl status $(SERVICE_NAME)"
	@echo "Logs:    journalctl -u $(SERVICE_NAME) -f"
	@echo "------------------------------------------------------"

uninstall:
	systemctl stop $(SERVICE_NAME) || true
	systemctl disable $(SERVICE_NAME) || true
	rm -f $(BINDIR)/$(TARGET)
	rm -f $(SERVICEDIR)/$(SERVICE_NAME)
	systemctl daemon-reload
	@echo "Uninstalled successfully."
