
include $(TOPDIR)/rules.mk
export OM_DEBUG=

PKG_NAME:=firmware-ev
PKG_VERSION:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_URL:=
PKG_MD5SUM:=
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/firmware-ev
  SUBMENU:=Distributions
  SECTION:=net
  CATEGORY:=Network
  TITLE:=zero-config VPN/MESH network
  URL:=
  MAINTAINER:=
  DEPENDS:=+libuci +libnl-tiny +libpolarssl +libev
endef

define Package/firmware-ng/description
zero-config network
endef

define Package/firmware-ng/config
	select BUSYBOX_CONFIG_NOHUP
	select BUSYBOX_CONFIG_RUN_PARTS
	select BUSYBOX_CONFIG_IP
	select BUSYBOX_CONFIG_FEATURE_IP_ADDRESS
	select BUSYBOX_CONFIG_FEATURE_IP_LINK
	select BUSYBOX_CONFIG_FEATURE_IP_ROUTE
	select BUSYBOX_CONFIG_FEATURE_IP_TUNNEL
	select BUSYBOX_CONFIG_FEATURE_IP_RULE
	select BUSYBOX_CONFIG_FEATURE_IP_SHORT_FORMS
	select BUSYBOX_CONFIG_FEATURE_IP_RARE_PROTOCOLS
#	select BUSYBOX_CONFIG_UDHCPC_DEFAULT_SCRIPT="/sbin/udhcpc-event"
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/ $(PKG_BUILD_DIR)/src
	$(CP) ./files/ $(PKG_BUILD_DIR)/files
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
	chmod -R 777 $(PKG_BUILD_DIR)/files/etc/init.d/ev_boot
	chmod -R 777 $(PKG_BUILD_DIR)/files/root/*
endef

define Package/firmware-ev/install
	$(INSTALL_DIR) $(1)/ $(1)/bin
	$(CP) ./files/* $(1)/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/ev $(1)/bin/
endef

$(eval $(call BuildPackage,firmware-ev))
