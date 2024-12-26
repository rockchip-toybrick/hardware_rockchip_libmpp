# Makefile for generating compile-time and Git version information

# Variables for time information
CURRENT_YEAR := $(shell date -u +"%Y")
CURRENT_TIME := $(shell date -u +"%Y-%m-%d %H:%M:%S")

# Variables for Git version information
VERSION_CNT := 0
VERSION_MAX_CNT := 9
VERSION_INFO := "unknown mpp version for missing VCS info"
VERSION_HISTORY :=

# Check if .git exists
ifneq (,$(wildcard .git))
    GIT := $(shell which git)
    ifneq (,$(GIT))
        # Get current version info
        GIT_LOG_FORMAT := "%h author: %<(30)%an %cd %s"
        VERSION_INFO := $(shell $(GIT) log -1 --oneline --date=short --pretty=format:$(GIT_LOG_FORMAT))
        ifneq (,$(VERSION_INFO))
            VERSION_INFO := \"$(shell echo $(VERSION_INFO) | sed 's/"/\\"/g')\"
        endif

        # Get version history
        VERSION_HISTORY :=
        $(foreach CNT,$(shell seq 0 $(VERSION_MAX_CNT)),\
            $(eval LOG := $(shell $(GIT) log HEAD~$(CNT) -1 --oneline --date=short --pretty=format:$(GIT_LOG_FORMAT))) \
            $(if $(strip $(LOG)),\
                $(eval VERSION_HISTORY_$(VERSION_CNT) := "$(LOG)"") \
                $(eval VERSION_CNT := $(shell echo $$(( $(VERSION_CNT) + 1 )))) \
            )\
        )
    endif
endif

# Generate version.h
VERSION_H := $(TOP)/include/version.h
define generate_version
    echo Generating version.h...
    @mkdir -p include
    @echo "#ifndef __VERSION_H__" > $(VERSION_H)
    @echo "#define __VERSION_H__" >> $(VERSION_H)
    @echo " " >> $(VERSION_H)
    @echo "#define CURRENT_TIME       \"$(CURRENT_TIME)\"" >> $(VERSION_H)
    @echo " " >> $(VERSION_H)
    @echo "#define KMPP_VERSION       $(VERSION_INFO)" >> $(VERSION_H)
    @echo "#define KMPP_VER_HIST_CNT  $(VERSION_CNT)" >> $(VERSION_H)
    $(foreach N,$(shell seq 0 $(shell echo $(VERSION_CNT)-1 | bc)), \
        echo "#define KMPP_VER_HIST_$(N)    \"$(VERSION_HISTORY_$(N))\" >> $(VERSION_H); \
    )
    @echo " " >> $(VERSION_H)
    @echo "#endif // __VERSION_H__" >> $(VERSION_H)
endef
