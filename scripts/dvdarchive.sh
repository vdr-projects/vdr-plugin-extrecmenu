#!/bin/bash
#
# Version 1.1 2006-03-27
#
# Exitcodes:
#
# exit 0 - no error
# exit 1 - mount/umount error
# exit 2 - no dvd in drive
# exit 3 - wrong dvd in drive / recording not found
# exit 4 - error while linking [0-9]*.vdr
#
# Errorhandling/Symlinking: vejoun@vdr-portal
#
# For dvd-in-drive detection download isodetect.c, compile it and put it into the PATH,
# usually /usr/local/bin/
#

#<Configuration>

MOUNTCMD="/usr/bin/sudo /bin/mount"
UMOUNTCMD="/usr/bin/sudo /bin/umount"
MOUNTPOINT="/media/cdrom" # no trailing '/'!

#</Configuration>

DEVICE="$(grep "$MOUNTPOINT" /etc/fstab | head -n1 | awk '{ print $1; }')" # dvd-device, used by isodetect if exists

REC="$2"
NAME="$3"

case "$1" in
mount)
	# check if dvd is in drive, only if isodetect exists
	if [ -n "$(which isodetect)" -a -n "$DEVICE" ]; then
		isodetect -d "$DEVICE" >/dev/null 2>&1
		if [ $? -ne 0 ]; then
			echo "no dvd in drive"
			exit 2
		fi
	fi
	# check if not mounted
	$MOUNTCMD | grep "$MOUNTPOINT" >/dev/null && { echo "dvd already mounted"; exit 1; }
	# mount dvd
 	$MOUNTCMD "$MOUNTPOINT" || { echo "dvd mount error"; exit 1; }
 	# is mounted?
	# find recording on dvd
	DIR="$(find "${MOUNTPOINT}/" -name "$NAME")"
	# if not found, umount
	if [ -z "$DIR" ]; then
		$UMOUNTCMD "$MOUNTPOINT" || { echo "dvd umount error"; exit 1; }
		echo "wrong dvd in drive / recording not found on dvd"
		exit 3
	fi
	# link index.vdr if not exist
	if [ ! -e "${REC}/index.vdr" ]; then
		cp -s "${DIR}/index.vdr" "${REC}/"
	fi
	# link [0-9]*.vdr files
	cp -s "${DIR}/"[0-9]*.vdr "${REC}/"
	# error while linking [0-9]*.vdr files?
	if [ $? -ne 0 ]; then
		# umount dvd bevor unlinking
		$UMOUNTCMD "$MOUNTPOINT" || { echo "dvd umount error"; exit 1; }
		# unlink broken links
		for LINK in "${REC}/"*.vdr; do
			if [ -L "$LINK" -a ! -s "$LINK" ]; then
				rm "$LINK"
			fi
		done
		echo "error while linking [0-9]*.vdr"
		exit 4
	fi
	;;
umount)
	# check if dvd is mounted
	$MOUNTCMD | grep "$MOUNTPOINT" >/dev/null || { echo "dvd not mounted"; exit 1; }
	# is mounted?
	# umount dvd bevor unlinking
	$UMOUNTCMD "$MOUNTPOINT" || { echo "dvd umount error"; exit 1; }
	# unlink broken links
	for LINK in "${REC}/"*.vdr; do
		if [ -L "$LINK" -a ! -s "$LINK" ]; then
			rm "$LINK"
		fi
	done
	;;
esac

exit 0
