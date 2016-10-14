
include $(TOPDIR)/rules.mk

PKG_NAME:=ifplugd
PKG_VERSION:=0.28
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://0pointer.de/lennart/projects/ifplugd
PKG_MD5SUM:=df6f4bab52f46ffd6eb1f5912d4ccee3

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/ifplugd
  SECTION:=net
  CATEGORY:=Network
  TITLE:=Network Interface Plug Detection Daemon
  URL:=http://0pointer.de/lennart/projects/ifplugd/
  DEPENDS:=+libdaemon
endef

CONFIGURE_ARGS += \
       --disable-lynx \
       --disable-xmltoman \
       --disable-subversion \

define Package/ifplugd/install
	$(INSTALL_DIR) $(1)/etc/init.d $(1)/etc/rc.d $(1)/usr/sbin
	$(CP) ./files/* $(1)/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/ifplugd $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/ifplugstatus $(1)/usr/sbin/
endef

$(eval $(call BuildPackage,ifplugd))
