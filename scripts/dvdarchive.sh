#!/bin/bash
#
# Version 1.4 2006-04-07
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
# Tools needed: mount, awk, find, test
# Optional tools: isodetect

#<Configuration>

MOUNTCMD="/usr/bin/sudo /bin/mount"
UMOUNTCMD="/usr/bin/sudo /bin/umount"

MOUNTPOINT="/media/cdrom" # no trailing '/'!

# Eject DVD for exit-codes 2 and 3 (no or wrong dvd). 1 = yes, 0 = no.
EJECTWRONG=0
# Eject DVD after unmounting. 1 = yes, 0 = no.
EJECTUMOUNT=0

#</Configuration>

DEVICE="$(awk '( $1 !~ /^#/ ) && ( $2 == "'$MOUNTPOINT'" ) { printf("%s", $1); exit; }' /etc/fstab)" # dvd-device, used by isodetect if exists

REC="$2"
NAME="$3"

call() {
	echo -e "\nScript $0 needs three parameters. Action, rec and name. Action is mount or umount"
	echo -e "Example: dvdarchive.sh mount '/video1.0/Music/%Riverdance/2004-06-06.00:10.50.99.rec' '2004-06-06.00:10.50.99.rec'\n"
}

[ $# -ne 3 ] && { call; exit 10; }

case "$1" in
mount)
	# check if dvd is in drive, only if isodetect exists
	if [ -n "$(which isodetect)" -a -n "$DEVICE" ]; then
		isodetect -d "$DEVICE" >/dev/null 2>&1
		if [ $? -ne 0 ]; then
			echo "no dvd in drive"
			[ $EJECTWRONG -eq 1 ] && { eject "$DEVICE"; }
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
		[ $EJECTWRONG -eq 1 ] && { eject "$DEVICE"; }
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
	[ $EJECTUMOUNT -eq 1 ] && { eject "$DEVICE"; }
	;;
     *)
        echo -e "\nWrong action."
        call
        ;;
esac

exit 0
