
include $(TOPDIR)/rules.mk
export EV_DEBUG=

PKG_NAME:=libev
PKG_VERSION:=0.1

#PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_URL:=
PKG_MD5SUM:=
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/libev
  NAME:=libev
  SECTION:=libs
  CATEGORY:=Libraries
  SUBMENU:=Distributions
  TITLE:=zero-config network library
  #URL:=
  DEPENDS:=+libuci +libpolarssl +libcurl +librt +libjson-c +libnl-tiny +jansson
endef

define Package/libev/description
zero-config mesh network library
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/ $(PKG_BUILD_DIR)/src
endef

define Build/Configure
endef

MAKE_FLAGS += \
	OFLAGS="$(TARGET_CFLAGS) $(TARGET_LDFLAGS)" \
	CC="$(TARGET_CC)" \
	STRIP="/bin/true"

TARGET_CFLAGS:= \
	-I$(STAGING_DIR)/usr/include/libnl-tiny \
	-I$(STAGING_DIR)/usr/include/ \
	$(TARGET_CFLAGS)

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)/src $(MAKE_FLAGS)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig/ $(1)/usr/lib/ $(1)/usr/include/libev/
	$(CP) $(PKG_BUILD_DIR)/src/libev.so* $(1)/usr/lib/
	$(CP) $(PKG_BUILD_DIR)/src/libev.pc $(1)/usr/lib/pkgconfig/
	$(CP) $(PKG_BUILD_DIR)/src/include/libev/* $(1)/usr/include/libev/
endef

define Package/libev/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/src/libev.so* $(1)/usr/lib/
endef

$(eval $(call BuildPackage,libev))
