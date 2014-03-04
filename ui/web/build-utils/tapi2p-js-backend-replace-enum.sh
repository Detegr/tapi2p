#!/bin/sh

# $1 = core/event.h
# $2 = web/ui/js/tapi2p-backend.js
# $3 = tapi2p-backend.js output

EVENT_ENUM=$(awk '/typedef enum {/,/EventCount/ {
	if ($0 ~ /\t.*/) {
		if($0 !~ /\tEventCount/) {
			gsub("\t", "");
			gsub(",", "\": ENUM_BASE++,\\");
			gsub(" ", "\\ ");
			print "\"" $0;
		}
	}
}' $1)
SED_ARG='s/TAPI2P_REPLACE_COMMANDS_FROM_EVENT_H/'"${EVENT_ENUM%?}"'/'
sed $2 -e "$SED_ARG" > $3
