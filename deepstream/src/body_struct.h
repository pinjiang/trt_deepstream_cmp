#ifndef BODY_POSE_STRUCT_H
#define BODY_POSE_STRUCT_H

#define NUM_BODY_KEYPOINTS   24
#define TYPE_BODY_KEYPOINTS 0x104

typedef struct tCoords {
  float x;
  float y;
  float z;
}Coords;

typedef struct tBodyKeyPointsCoords {
  Coords keypoints[NUM_BODY_KEYPOINTS];
}BodyKeyPointsCoords;

#endif
