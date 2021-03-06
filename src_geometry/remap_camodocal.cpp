#include <iostream>
#include <string>

#include "camodocal/camera_models/Camera.h"
#include "camodocal/camera_models/CameraFactory.h"
#include "camodocal/camera_models/CataCamera.h"
#include "camodocal/camera_models/EquidistantCamera.h"

//opencv
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "../utils/MiscUtils.h"
#include "../utils/ElapsedTime.h"
#include "../utils/PoseManipUtils.h"
#include "../utils/TermColor.h"
#include "../utils/CameraGeometry.h"

// For a camera gets a K
void getK( camodocal::CameraPtr m_cam, Matrix3d& K )
{
    Matrix3d K_rect;
    vector<double> m_camera_params;
    m_cam->writeParameters( m_camera_params ); // retrive camera params from Abstract Camera.
    // camodocal::CataCamera::Parameters p();
    // cout << "size=" << m_camera_params.size() << " ::>\n" ;
    // for( int i=0 ; i<m_camera_params.size() ; i++ ) cout << "\t" << i << " " << m_camera_params[i] << endl;

    switch( m_cam->modelType() )
    {
        case camodocal::Camera::ModelType::MEI:
            K_rect << m_camera_params[5], 0, m_camera_params[7],
                      0, m_camera_params[6], m_camera_params[8],
                      0, 0, 1;
            break;
        case camodocal::Camera::ModelType::PINHOLE:
            K_rect << m_camera_params[4], 0, m_camera_params[6],
                      0, m_camera_params[5], m_camera_params[7],
                      0, 0, 1;
            break;
        case camodocal::Camera::ModelType::KANNALA_BRANDT:
            K_rect << m_camera_params[4], 0, m_camera_params[6],
                      0, m_camera_params[5], m_camera_params[7],
                      0, 0, 1;
            break;
            default:
            // TODO: Implement for other models. Look at initUndistortRectifyMap for each of the abstract class.
            cout << "[getK] Wrong\nQuit....";
            exit(10);

    }
    K = Matrix3d(K_rect);
}


// new K
void make_K( float new_fx, float new_fy, float new_cx, float new_cy, Matrix3d& K )
{
    // Pass these as arguments
    // float new_fx = 375.;
    // float new_fy = 375.;
    // float new_cx = 376.;
    // float new_cy = 240.;

    K << new_fx, 0, new_cx,
              0, new_fy, new_cy,
              0, 0, 1;

}


void do_image_undistortion( camodocal::CameraPtr m_cam, Matrix3d new_K, const cv::Mat& im_raw, cv::Mat & im_undistorted )
{
    float new_fx = new_K(0,0); // 375.;
    float new_fy = new_K(1,1); //375.;
    float new_cx = new_K(0,2); //376.;
    float new_cy = new_K(1,2); //240.;

    cv::Mat map_x, map_y;
    m_cam->initUndistortRectifyMap( map_x, map_y, new_fx, new_fy, cv::Size(0,0), new_cx, new_cy ); // inefficient. but i don't care now.TODO ideally should create the map only once and reuse it.
    cv::remap( im_raw, im_undistorted, map_x, map_y, CV_INTER_LINEAR );
}


void compute_stereo_rectification_transform( const Matrix3d& K_new, const Matrix4d& right_T_left, cv::Size imsize,
    cv::Mat& R1, cv::Mat& R2,
    cv::Mat& P1, cv::Mat& P2
                )
{
    IOFormat numpyFmt(FullPrecision, 0, ", ", ",\n", "[", "]", "[", "]");


    // Adopted from : http://homepages.inf.ed.ac.uk/rbf/CVonline/LOCAL_COPIES/FUSIELLO/node18.html
    cout << "[compute_stereo_rectification_transform]" << endl;
    cout << TermColor::GREEN();
    cv::Mat K_new_cvmat;
    cv::eigen2cv( K_new, K_new_cvmat );
    cout << "K_new_cvmat " << MiscUtils::cvmat_info( K_new_cvmat ) << endl;;

    // cv::Mat D = cv::Mat::zeros( 4,1, CV_64FC1 );
    // cout << "D " << MiscUtils::cvmat_info( D ) << endl;;
    cv::Mat D;

    Matrix3d right_R_left = right_T_left.topLeftCorner(3,3);
    Vector3d right_t_left = right_T_left.col(3).topRows(3);
    cv::Mat R, T;
    cv::eigen2cv( right_R_left, R );
    cv::eigen2cv( right_t_left, T );
    cout << "R " << MiscUtils::cvmat_info( R ) << endl;
    cout << "T " << MiscUtils::cvmat_info( T ) << endl;

    cout << "cv::Size w,h = " << imsize.width << " " << imsize.height << endl;

    // cv::Mat R1, R2;
    // cv::Mat P1, P2;
    cv::Mat Q;
    MatrixXd e_R1, e_R2, e_P1, e_P2, e_Q;
    cv::stereoRectify( K_new_cvmat, D, K_new_cvmat, D, imsize, R, T,
                        R1, R2, P1, P2, Q
                    );
    cv::cv2eigen( R1, e_R1 );
    cv::cv2eigen( R2, e_R2 );
    cv::cv2eigen( P1, e_P1 );
    cv::cv2eigen( P2, e_P2 );
    cv::cv2eigen( Q, e_Q );

    cout << "R1=" << e_R1.format(numpyFmt) << endl;
    cout << "R2=" << e_R2.format(numpyFmt) << endl;
    cout << "P1=" << e_P1.format(numpyFmt) << endl;
    cout << "P2=" << e_P2.format(numpyFmt) << endl;
    cout << TermColor::RESET();
    cout << "[/compute_stereo_rectification_transform]" << endl;

}


/*
void get_rectification_map( const Matrix3d& K_new, const cv::Mat& R1,
                                cv::Size imsize,
                                cv::Mat& map_x, cv::Mat& map_y )
{
    cout << "[get_rectification_map]" << endl;
    cv::Mat K_new_cvmat;
    cv::eigen2cv( K_new, K_new_cvmat );
    cout << "K_new_cvmat " << MiscUtils::cvmat_info( K_new_cvmat ) << endl;;

    cv::Mat D = cv::Mat::zeros( 4,1, CV_64FC1 );
    cout << "D " << MiscUtils::cvmat_info( D ) << endl;

    // C++: void initUndistortRectifyMap( cameraMatrix,  distCoeffs,  R,  newCameraMatrix,  size,  m1type,  map1,  map2)
    cv::initUndistortRectifyMap( K_new_cvmat, D, R1, K_new_cvmat, imsize, CV_32FC1, map_x, map_y );

    cout << TermColor::RESET();
    cout << "[/get_rectification_map]" << endl;
}
*/

void get_rectification_map( const Matrix3d& K_new, const cv::Mat& R1, const cv::Mat& P1,
                                cv::Size imsize,
                                cv::Mat& map_x, cv::Mat& map_y )
{
    cout << "[get_rectification_map]" << endl;
    cv::Mat K_new_cvmat;
    cv::eigen2cv( K_new, K_new_cvmat );
    cout << "K_new_cvmat " << MiscUtils::cvmat_info( K_new_cvmat ) << endl;;

    cv::Mat D;// = cv::Mat::zeros( 4,1, CV_64FC1 );
    // cout << "D " << MiscUtils::cvmat_info( D ) << endl;

    // C++: void initUndistortRectifyMap( cameraMatrix,  distCoeffs,  R,  newCameraMatrix,  size,  m1type,  map1,  map2)
    cv::initUndistortRectifyMap( K_new_cvmat, D, R1, P1, imsize, CV_32FC1, map_x, map_y );

    cout << TermColor::RESET();
    cout << "[/get_rectification_map]" << endl;
}

void normalize_disparity_for_vis( const cv::Mat& disp_raw, cv::Mat& disp8 )
{
    // given disp raw from sgbm (CV_16UC1) which also has negative values. normaloze it .
    cv::normalize(disp_raw, disp8, 0, 255, CV_MINMAX, CV_8U);
    return;
    disp8 = cv::Mat::zeros( disp_raw.rows, disp_raw.cols, CV_8UC1 );
    for( int i=0 ; i<disp_raw.rows ; i++ ) {
        for( int j=0 ; j<disp_raw.cols; j++ ) {
            if( disp_raw.at<int16_t>(i,j) > 0 )
                disp8.at<uchar>(i,j) = (uchar) disp_raw.at<int16_t>(i,j);
        }
    }
}


// Trying out Epipolar Geometry with camodocal.
// Theory Background : http://homepages.inf.ed.ac.uk/rbf/CVonline/LOCAL_COPIES/FUSIELLO/tutorial.html
// Note: OpenCV's functions are extremely confusing and best not used. These things are actually a lot
// simpler than seem to be by looking at opencv's function.
//
// Here is what I learnt:
// - Given raw images undistort the images. When you undistort you can set the camera intrinsic new to arbitary values. See my K_new for example.
// - After undisort, you can as well forget about camera model and just use K_new
// - For stereo pair you want to set the same K_new for both left and right image. K_new is essentially just scaling the normalized co-ordinates to pixels.
// - Fundamental matrix from given R,T. F = K_new.transpose().inverse() * Tx * right_T_left.topLeftCorner(3,3) * K_new.inverse();
// - Epipoles are the nullspaces of matrix F and F.transpose().
// - x <---> ld=Fx and l=F.transpose() x <---> xd.
int main()
{
    // const std::string BASE = "/Bulk_Data/_tmp_cerebro/mynt_multi_loops_in_lab/";
    const std::string BASE = "/Bulk_Data/ros_bags/bluefox_stereo/calib/leveled_cam_sampled/";
    // const std::string BASE = "/Bulk_Data/ros_bags/bluefox_stereo/calib/right_titled_sampled/";

    IOFormat numpyFmt(FullPrecision, 0, ", ", ",\n", "[", "]", "[", "]");



    camodocal::CameraPtr m_camera_left, m_camera_right;
    // m_camera_left = camodocal::CameraFactory::instance()->generateCameraFromYamlFile(BASE+"/cameraIntrinsic.0.yaml");
    // m_camera_right = camodocal::CameraFactory::instance()->generateCameraFromYamlFile(BASE+"/cameraIntrinsic.1.yaml");
    // m_camera_left = camodocal::CameraFactory::instance()->generateCameraFromYamlFile(string("/app/catkin_ws/src/vins_mono_debug_pkg/build/stereo_calib2_pinhole")+"/camera_left.yaml");
    // m_camera_right = camodocal::CameraFactory::instance()->generateCameraFromYamlFile(string("/app/catkin_ws/src/vins_mono_debug_pkg/build/stereo_calib2_pinhole")+"/camera_right.yaml");
    m_camera_left = camodocal::CameraFactory::instance()->generateCameraFromYamlFile(string(BASE+"../leveled_cam_pinhole/")+"/camera_left.yaml");
    m_camera_right = camodocal::CameraFactory::instance()->generateCameraFromYamlFile(string(BASE+"../leveled_cam_pinhole/")+"/camera_right.yaml");
    // m_camera_left = camodocal::CameraFactory::instance()->generateCameraFromYamlFile(string(BASE+"../right_titled_pinhole/")+"/camera_left.yaml");
    // m_camera_right = camodocal::CameraFactory::instance()->generateCameraFromYamlFile(string(BASE+"../right_titled_pinhole/")+"/camera_right.yaml");

    cout << m_camera_left->parametersToString() << endl;
    cout << m_camera_right->parametersToString() << endl;

    //
    // Intrinsics
    // Camera intrinsics from camera calibration files. These have no significance in stereo computation. As well not get these.
    /*
    Matrix3d K, Kd;
    getK( m_camera_left, K );
    getK( m_camera_right, Kd );
    cout << "K\n"<< K << endl;
    cout << "Kd\n"<< Kd << endl;
    */

    // I can set new intrinsic to whatever I want after undistortion.
    // This has been a major point of confusion. Note that K_new just scales the normalized image co-ordinates (irrespective of camera model) to pixels.
    // In case of a stereo pair you want both your camera to have the same scaling.
    Matrix3d K_new;
    make_K( 375.0, 375.0, 376.0, 240.0, K_new ); // this can be any arbitrary values, but usually you want to have fx,fy in a similar range to original intrinsic and cx,cy as half of image width and height respectively so that you can view the image correctly
    cout << "K_new="<< K_new.format(numpyFmt) << endl;

    //
    // Extrinsic (Baseline)
    Matrix4d right_T_left = Matrix4d::Zero();
    // from mynt
    // double _rot_xyzw[] = {-1.8252509868889259e-04,-1.6291774489779708e-03,-1.2462127842978489e-03,9.9999787970731446e-01};
    // double _tr_xyz[] = {-1.2075905420832895e+02/1000.,5.4110610639412482e-01/1000.,2.4484815673909591e-01/1000.};

    // from bluefox_stereo leveled
    double _rot_xyzw[] = { -1.7809713490350254e-03, 4.2143149583451564e-04,4.1936662160154632e-02, 9.9911859501433165e-01};
    double _tr_xyz[] = {-1.4031938291177164e+02/1000.,-6.6214729932530441e+00/1000.,1.4808567571722902e+00/1000.};

    // from bluefox_stereo right-titled
    // double _rot_xyzw[] = {7.6402817795312578e-02, -2.5540621037530589e-02,-3.6647229516532570e-01, 9.2693491841995723e-01};
    // double _tr_xyz[] = { -1.1921506144031111e+01/1000.,6.3120089648277940e+01/1000.,-1.8124558314119258e+02/1000.};

    PoseManipUtils::raw_xyzw_to_eigenmat( _rot_xyzw, _tr_xyz, right_T_left );
    cout << "right_T_left:" << PoseManipUtils::prettyprintMatrix4d( right_T_left ) << endl;
    cout << "right_Rot_left=" << right_T_left.topLeftCorner(3,3).format(numpyFmt) << endl;
    cout << "right_trans_left=" << right_T_left.col(3).topRows(3).format(numpyFmt) << endl;

    //
    // Load raw image
    int frame_id = 20;
    // cv::Mat im_left =  cv::imread( BASE+"/"+std::to_string(frame_id)+".jpg" );
    // cv::Mat im_right = cv::imread( BASE+"/"+ std::to_string(frame_id)+"_1.jpg" );
    cv::Mat im_left =  cv::imread( BASE+"/cam0_"+std::to_string(frame_id)+".png" );
    cv::Mat im_right = cv::imread( BASE+"/cam1_"+ std::to_string(frame_id)+".png" );

    cv::Mat im_left_undistorted, im_right_undistorted;
    do_image_undistortion( m_camera_left, K_new, im_left, im_left_undistorted );
    do_image_undistortion( m_camera_right, K_new, im_right, im_right_undistorted );



    // cout << "right_t_left: " << right_T_left.col(3).topRows(3).transpose() << endl;
    Matrix3d Tx;
    PoseManipUtils::vec_to_cross_matrix( right_T_left.col(3).topRows(3), Tx );
    // cout << "Tx" << Tx << endl;

    // Matrix3d F = Kd.transpose().inverse() * Tx * right_T_left.topLeftCorner(3,3) * K.inverse();  //< Fundamental Matrix
    Matrix3d F = K_new.transpose().inverse() * Tx * right_T_left.topLeftCorner(3,3) * K_new.inverse();  //< Fundamental Matrix
    cout << "F="  << F.format(numpyFmt) << endl;


    // tryout multiple points and get their corresponding epipolar line.
    #if 1
    for( int i=0 ; i<500; i+=10 ) {
    #if 1
    // take a sample point x on left image and find the corresponding line on the right image
    Vector3d x(1.5*i, i, 1.0);
    Vector3d ld = F * x;
    MiscUtils::draw_point( x, im_left_undistorted, cv::Scalar(255,0,255) );
    MiscUtils::draw_line( ld, im_right_undistorted, cv::Scalar(255,0,255) );
    #endif

    #if 1
    // take a sample point on right image and find the corresponding line on the left image
    Vector3d xd(i, i, 1.0);
    Vector3d l = F.transpose() * xd;
    MiscUtils::draw_line( l, im_left_undistorted, cv::Scalar(0,0,255) );
    MiscUtils::draw_point( xd, im_right_undistorted, cv::Scalar(0,0,255) );
    #endif


    cv::imshow( "left", im_left );
    cv::imshow( "right", im_right );
    cv::imshow( "im_left_undistorted", im_left_undistorted );
    cv::imshow( "im_right_undistorted", im_right_undistorted );
    cv::waitKey(10);
    }
    #endif

    //------------------------------------ Stereo Rectify --------------------------------------//
    // Adopted from : http://homepages.inf.ed.ac.uk/rbf/CVonline/LOCAL_COPIES/FUSIELLO/node18.html
    // TODO : Stereo Rectify images and check epipolar geometry again

    // Matrix<double,3,4> P_old_left = K_new * Matrix4d::Identity().topRows( 3 );
    // Matrix<double,3,4> P_old_right = K_new * right_T_left.topRows( 3 );
    // cout << "P_old_left=" << P_old_left.format(numpyFmt) << endl;
    // cout << "P_old_right=" << P_old_right.format(numpyFmt) << endl;

    //-----------------a
    cv::Mat R1, R2;
    cv::Mat P1, P2;
    cv::Size imsize = cv::Size( m_camera_left->imageWidth(), m_camera_left->imageHeight());
    compute_stereo_rectification_transform( K_new, right_T_left, imsize,
                R1, R2,
                P1, P2
                );

    // cout << "R1\n" << R1 << endl;
    // cout << "R2\n" << R2 << endl;

    //----------------b
    cv::Mat map1_x, map1_y;
    cv::Mat map2_x, map2_y;
    cv::Size imsize_super = cv::Size( m_camera_left->imageWidth(), m_camera_left->imageHeight());
    get_rectification_map( K_new, R1, P1, imsize_super, map1_x, map1_y );
    get_rectification_map( K_new, R2, P2, imsize_super, map2_x, map2_y );


    //----------------c
    cv::Mat im_left_undistorted_srectified, im_right_undistorted_srectified;
    cv::remap( im_left_undistorted, im_left_undistorted_srectified, map1_x, map1_y, CV_INTER_LINEAR );
    cv::remap( im_right_undistorted, im_right_undistorted_srectified, map2_x, map2_y, CV_INTER_LINEAR );

    cv::imshow( "im_left_undistorted", im_left_undistorted );
    cv::imshow( "im_right_undistorted", im_right_undistorted );

    cv::imshow( "im_left_undistorted_srectified", im_left_undistorted_srectified );
    cv::imshow( "im_right_undistorted_srectified", im_right_undistorted_srectified );
    cv::waitKey(0);

    // TODO: Need to verify that the epipolar lines are horizontal after this mapping

    //------------------------------- SGBM -------------------------------------------------//
    cv::Ptr<cv::StereoSGBM> sgbm;
    sgbm = cv::StereoSGBM::create(0,16,3);
    cv::Mat hu;
    sgbm->compute( im_left_undistorted_srectified, im_right_undistorted_srectified, hu );
    cout << "hu " << MiscUtils::cvmat_info( hu ) << endl;
    cout << hu << endl;
    cv::Mat hu8;
    normalize_disparity_for_vis( hu, hu8);
    cv::imshow( "hu" , hu8 );
    cv::waitKey(0);
}



// Trying out undistort
/*
int main()
{
    const std::string base = "/Bulk_Data/_tmp_cerebro/tum_magistrale1/";
    // std::string calib_file = "../examples/sample_kannala_brandt.yaml";
    // std::string calib_file = "../examples/sasmple_pinhole.yaml";
    std::string calib_file = base+"/cameraIntrinsic.yaml";


    camodocal::CameraPtr m_camera;
    std::cout << ((m_camera)?"cam is initialized":"cam is not initiazed") << std::endl; //this should print 'not initialized'

    m_camera = camodocal::CameraFactory::instance()->generateCameraFromYamlFile(calib_file);
    std::cout << ((m_camera)?"cam is initialized":"cam is not initiazed") << std::endl; //this should print 'initialized'

    // Inbuild function for priniting
    std::cout << m_camera->parametersToString() << std::endl;

    // Custom priniting info
    std::cout << "imageWidth="  << m_camera->imageWidth() << std::endl;
    std::cout << "imageHeight=" << m_camera->imageHeight() << std::endl;
    std::cout << "model_type=" << m_camera->modelType() << std::endl;
    std::cout << "camera_name=" << m_camera->cameraName() << std::endl;


    cv::Mat map_x, map_y;
    m_camera->initUndistortRectifyMap( map_x, map_y );


    cout << "map_x, map_y: " << map_x.at<float>(50,60) << " " << map_y.at<float>(50,60) << endl;
    Vector3d outSp;
    m_camera->liftProjective( Vector2d(60.0,50.0), outSp ); // step-1: K_inv * x , step-2: apply inverse distortion ==> normalized image co-ordinates
    outSp = outSp / outSp(2);
    cout << "outSp: " << outSp << endl;

    Matrix3d K_rect;
    vector<double> m_camera_params;
    m_camera->writeParameters( m_camera_params );
    // camodocal::CataCamera::Parameters p();
    cout << "size=" << m_camera_params.size() << " ::> " << m_camera_params[0] << " " << m_camera_params[1] << endl;
    K_rect << m_camera_params[4], 0, 512. / 2,
              0, m_camera_params[5], 512. / 2,
              0, 0, 1;


    // Vector3d outSp__1 = K_rect.inverse() * outSp ;
    Vector3d outSp__1 = K_rect.inverse() * Vector3d( 60,50,1.) ;
    Vector2d outSp__2;
    m_camera->spaceToPlane( outSp__1, outSp__2  );
    // cout << K_rect.inverse() * outSp << endl;
    cout << "outSp__2: " << outSp__2 << endl; // this is exactly equal to map_x, map_y at that point.


    Vector2d __f;
    m_camera->spaceToPlane( outSp , __f );
    cout << "__f:" <<  __f << endl;

    return 0;



    // cv::remap
    for( int i=0 ; i<1000 ; i++ ) {
    // cv::Mat im = cv::imread( base+"100.jpg");
    cv::Mat im = cv::imread( base+std::to_string(i)+".jpg");
    cv::imshow( "org", im );
    // cv::waitKey(0);


    std::cout << MiscUtils::cvmat_info( map_x ) << std::endl;

    ElapsedTime timer;
    timer.tic();

    cv::Mat im_undist;
    cv::remap( im, im_undist, map_x, map_y, CV_INTER_LINEAR );
    std::cout << "im_undist : "<< MiscUtils::cvmat_info( im_undist ) << std::endl;

    std::cout << "cv::remap done in (ms) : " << timer.toc_milli() << endl;

    cv::imshow( "im_undist", im_undist  );
    cv::waitKey(0);
    }

}

*/
