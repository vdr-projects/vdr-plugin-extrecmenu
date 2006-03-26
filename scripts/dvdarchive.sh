#!/bin/bash

MOUNTCMD="/usr/bin/sudo /bin/mount"
UMOUNTCMD="/usr/bin/sudo /bin/umount"
MOUNTPOINT="/media/cdrom" # no trailing '/'!

PATH="$2"
NAME="$3"

case "$1" in
mount)
	# mount dvd
	$MOUNTCMD "$MOUNTPOINT"
	if [ $? -eq 0 ]
	then
		DIR="$(/usr/bin/find "${MOUNTPOINT}/" -name "$NAME")"
		# link vdr files
		/bin/cp -s "${DIR}/index.vdr" "${PATH}/"
		/bin/cp -s "${DIR}/"???.vdr "${PATH}/"
		if [ $? -ne 0 ]
		then
			$UMOUNTCMD "$MOUNTPOINT"
			# unlink broken links
			for LINK in "${PATH}/"*.vdr; do
				if [ -L "$LINK" -a ! -s "$LINK" ]; then
					/bin/rm "$LINK"
				fi
			done
			exit 2
		fi
        else
		exit 1
        fi
	;;
umount)
	$MOUNTCMD | /bin/grep "$MOUNTPOINT" > /dev/null
	if [ $? -eq 0 ]
	then
		# umount dvd
		$UMOUNTCMD "$MOUNTPOINT"
		# unlink broken links
		for LINK in "${PATH}/"*.vdr; do
			if [ -L "$LINK" -a ! -s "$LINK" ]; then
				/bin/rm "$LINK"
			fi
		done
	fi
	;;
esac
