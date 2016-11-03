
include $(TOPDIR)/rules.mk
export EV_DEBUG=

PKG_NAME:=dashboard
PKG_VERSION:=1

#PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_URL:=
PKG_MD5SUM:=
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/dashboard
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=package to communicate with cloud via dashboard
 #URL:=
  DEPENDS:=+libnl-tiny +libev +librt +libmosquitto
endef

define Package/dashboard/description
package to communicate with a dashboard
endef

define Package/dashboard/config
	select BUSYBOX_CONFIG_DIFF
	select BUSYBOX_CONFIG_IP
	select BUSYBOX_CONFIG_FEATURE_IP_ADDRESS
	select BUSYBOX_CONFIG_FEATURE_IP_LINK
	select BUSYBOX_CONFIG_FEATURE_IP_ROUTE
	select BUSYBOX_CONFIG_FEATURE_IP_TUNNEL
	select BUSYBOX_CONFIG_FEATURE_IP_RULE
	select BUSYBOX_CONFIG_FEATURE_IP_SHORT_FORMS
	select BUSYBOX_CONFIG_FEATURE_IP_RARE_PROTOCOLS
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/ $(PKG_BUILD_DIR)/src
endef

define Build/Configure
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

define Package/dashboard/install
	$(INSTALL_DIR) $(1)/ $(1)/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/dashboard $(1)/bin/
endef

$(eval $(call BuildPackage,dashboard))
