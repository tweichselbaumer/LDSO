add_library(ldso
        Feature.cc
        Frame.cc
        Point.cc
        Setting.cc
        Camera.cc
        Map.cc

		Win32/times.cpp

        internal/PointHessian.cc
        internal/FrameHessian.cc
        internal/GlobalCalib.cc
        internal/FrameFramePrecalc.cc
        internal/Residuals.cc
        internal/ImmaturePoint.cc
        internal/PR.cc

        internal/OptimizationBackend/AccumulatedSCHessian.cc
        internal/OptimizationBackend/AccumulatedTopHessian.cc
        internal/OptimizationBackend/EnergyFunctional.cc

        frontend/CoarseTracker.cc
        frontend/CoarseInitializer.cc
        frontend/FullSystem.cc
       # frontend/DSOViewer.cc
        frontend/FeatureDetector.cc
        frontend/FeatureMatcher.cc
        frontend/LoopClosing.cc
        frontend/PixelSelector2.cc
        frontend/Undistort.cc
        frontend/ImageRW_OpenCV.cc

		inertial/InertialFrameFrameHessian.cc
		inertial/InertialFrameHessian.cc
		inertial/InertialHessian.cc
		inertial/PreIntegration.cc
		inertial/InertialCoarseTrackerHessian.cc

		util/MatrixInverter.cc
)

target_link_libraries(
        ldso
        ${THIRD_PARTY_LIBS}
)
if (MSVC)
cotire(ldso)
endif (MSVC)
