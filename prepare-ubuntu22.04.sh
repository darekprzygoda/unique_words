docker build -t sandbox-22 -f Dockerfile-ubuntu22.04 .

here="$(readlink -m "$(dirname "${0}")")"

docker run --rm -it \
   -w "${here}" \
   -v "${here}:${here}" \
   -v "${HOME}/.Xauthority:${HOME}/.Xauthority" \
   -v /tmp/.X11-unix:/tmp/.X11-unix \
   -e DISPLAY="${DISPLAY}" -h "${HOSTNAME}" \
   sandbox-22