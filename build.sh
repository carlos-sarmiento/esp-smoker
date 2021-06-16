
FILES="base.yaml core-io.yaml pid-climate.yaml control-surfaces.yaml thermocouples.yaml display.yaml"

# uncommment if you want to DEBUG
# FILES="base.yaml core-io.yaml pid-climate.yaml control-surfaces.yaml thermocouples.yaml display.yaml debugging-components.yaml"

yq eval-all '. as $item ireduce ({}; . * $item )' $FILES  > smoker-controller.yaml

docker pull esphome/esphome:latest
docker run --rm -v "${PWD}":/config --device=/dev/ttyUSB0 -it esphome/esphome:latest smoker-controller.yaml run --upload-port /dev/ttyUSB0
