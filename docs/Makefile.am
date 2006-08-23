man_MANS = spkg.8

EXTRA_DIST = devel/Doxyfile devel/footer.html \
  devel/header.html devel/index.doc blogs/blog1.html \
  blogs/blog2.html blogs/blog3.html blogs/blog4.html \
  web/.htaccess web/common.php web/dloads.php web/docs.php \
  web/history.php web/howto.php web/index.php web/style.css \
  web/img/freshmeat.gif web/img/linuxlinks.gif web/dl/.htaccess \
  spkg.8

doxy:
	SRCDIR=$(top_srcdir) OUTDIR=$(builddir) \
	PROJECT="$(PACKAGE_NAME)" VERSION="$(PACKAGE_VERSION)" \
	  doxygen $(srcdir)/devel/Doxyfile
	rm -f api-html/doxygen.png

distclean-hook:
	rm -rf api-html
