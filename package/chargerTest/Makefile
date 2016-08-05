
include $(TOPDIR)/rules.mk
export EV_DEBUG=

PKG_NAME:=chargerTest
PKG_VERSION:=1

#PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
#TARGET_LDFLAGS+=/usr/include/openssl
PKG_BUILD_DEPENDS:=libopenssl
PKG_SOURCE_URL:=
PKG_MD5SUM:=
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/chargerTest
	SECTION:=utils
	CATEGORY:=Utilities
	TITLE:= chargerTest communicate with chargers
	DEPENDS:=+libopenssl +libc +libpthread +libuci +libnl-tiny +libev
endef

define Package/chargerTest/description
stay hungry stay foolish
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
	$(TARGET_LDFLAGS)


MAKE_FLAGS += \
	OFLAGS="$(TARGET_CFLAGS) $(TARGET_LDFLAGS)" \
	CC="$(TARGET_CC)" \
	STRIP="/bin/true"

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)/src $(MAKE_FLAGS)
endef
	
define Package/chargerTest/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_DIR) $(1)/ $(1)/bin
	$(CP) /lib/x86_64-linux-gnu/libc.so.6 $(1)/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/chargerTest $(1)/bin/
endef

$(eval $(call BuildPackage,chargerTest,+libopenssl,+libc,+libpthread,+libuci))

