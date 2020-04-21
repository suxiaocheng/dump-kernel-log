
# Copyright  2010, QNX Software Systems Ltd.  All Rights Reserved
#
# This source code has been published by QNX Software Systems Ltd.
# (QSSL).  However, any use, reproduction, modification, distribution
# or transfer of this software, or any software which includes or is
# based upon any of this code, is only permitted under the terms of
# the QNX Open Community License version 1.0 (see licensing.qnx.com for
# details) or as otherwise expressly authorized by a written license
# agreement from QSSL.  For more information, please email licensing@qnx.com.
#
ifndef QCONFIG
QCONFIG=qconfig.mk
endif
include $(QCONFIG)

define PINFO
PINFO DESCRIPTION=dump kernel log utils
endef

#===== LIBS - a space-separated list of library items to be included in the link.
include $(MKFILES_ROOT)/qmacros.mk

INSTALL_ROOT_nto=$(BASE)/install
USE_INSTALL_ROOT=1
INSTALLDIR=usr/bin

include $(MKFILES_ROOT)/qtargets.mk
LIBS +=screen
