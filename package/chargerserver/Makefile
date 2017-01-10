
include $(TOPDIR)/rules.mk

export EV_DEBUG=
##包名
PKG_NAME:=chargerserver
PKG_VERSION:=1

#PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
#TARGET_LDFLAGS+=/usr/include/openssl
PKG_BUILD_DEPENDS:=libopenssl
PKG_SOURCE_URL:=
PKG_MD5SUM:=
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

#包的目录
#包存放的目录下
#包的描述
#包的依赖库
define Package/chargerserver
	SECTION:=utils
	CATEGORY:=Utilities
	TITLE:= chargerserver --prints communicate with chargers
	DEPENDS:=+libopenssl +libc +libpthread +libuci +libnl-tiny +libev
endef

define Package/chargerserver/description
	chargerserver stay hungry stay foolish
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/ $(PKG_BUILD_DIR)/src
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) $(TARGET_CONFIGURE_OPTS)
endef

TARGET_CFLAGS:= \
	-I$(STAGING_DIR)/usr/include/libnl-tiny \
    	-I$(STAGING_DIR)/usr/include/ \
	$(TARGET_CFLAGS)

TARGET_LDFLAGS:= \
    	-lnl-tiny \
	-ljansson \
	$(TARGET_LDFLAGS)

MAKE_FLAGS += \
	OFLAGS="$(TARGET_CFLAGS) $(TARGET_LDFLAGS)" \
	CC="$(TARGET_CC)" \
	STRIP="/bin/true"

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)/src $(MAKE_FLAGS)
endef

###安装过程
###安装目录
###将包目录下的文件安装到openwrt下的bin目录下
define Package/chargerserver/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_DIR) $(1)/ $(1)/bin
	$(CP) /lib/x86_64-linux-gnu/libc.so.6 $(1)/bin
	$(CP) /usr/local/lib/libjansson* $(1)/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/chargerserver $(1)/bin/
endef

$(eval $(call BuildPackage,chargerserver,+libopenssl,+libc,+libpthread,+libuci))
