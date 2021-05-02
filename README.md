Compare the output tensor values from trt python script and deepstream app.<br />
The deepstream app is changed based on https://github.com/NVIDIA-AI-IOT/deepstream_pose_estimation.git<br />

Enviroment:
nvcr.io/nvidia/deepstream:5.1-21.02-triton

cd deepstream<br />
./download_model.sh # download the model<br />
make                # build the program ( if needed ) <br />
./run.sh            # run the app without publish the pose output <br />
./run_m.sh          # run the app publishing the pose output <br />

