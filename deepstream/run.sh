./deepstream-pose-app -i file://$PWD/../videos/pose.mp4 -o ../videos/output.mp4 -p /opt/nvidia/deepstream/deepstream-5.1/lib/libnvds_kafka_proto.so

# ./deepstream-pose-app -i file://$PWD/../videos/pose.mp4 -o ../videos/output.mp4 -p /opt/nvidia/deepstream/deepstream-5.1/lib/libnvds_kafka_proto.so -b -m nvmsgconv/libnvds_msgconv.so --conn-str='kafka;9092;deeppose-raw' -t deeppose-raw
