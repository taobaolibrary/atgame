#include "atgBase.h"
#include "atgMath.h"

const float VectorZero[3]   = { 0.0f,  0.0f,  0.0f };
const float VectorOne[3]    = { 1.0f,  1.0f,  1.0f };
const float VectorLeft[3]   = {-1.0f,  0.0f,  0.0f };
const float VectorRight[3]  = { 1.0f,  0.0f,  0.0f };
const float VectorUp[3]     = { 0.0f,  1.0f,  0.0f };
const float VectorDown[3]   = { 0.0f, -1.0f,  0.0f };
const float VectorForward[3]= { 0.0f,  0.0f,  1.0f };
const float VectorBack[3]   = { 0.0f,  0.0f, -1.0f };

const Vec3 Vec3One(VectorOne);
const Vec3 Vec3Zero(VectorZero);
const Vec3 Vec3Left(VectorLeft);
const Vec3 Vec3Right(VectorRight);
const Vec3 Vec3Up(VectorUp);
const Vec3 Vec3Down(VectorDown);
const Vec3 Vec3Forward(VectorForward);
const Vec3 Vec3Back(VectorBack);

const Vec4 Vec4One(1.0f, 1.0f, 1.0f, 1.0f);
const Vec4 Vec4Zero(0.0f, 0.0f, 0.0f, 0.0f);

const Quat QuatOne(1.0f, 1.0f, 1.0f, 1.0f);
const Quat QuatZero(0.0f, 0.0f, 0.0f, 0.0f);
const Quat QuatIdentity(0.0f, 0.0f, 0.0f, 1.0f);

const Matrix MatrixIdentity(1.0f, 0.0f, 0.0f, 0.0f, 
                             0.0f, 1.0f, 0.0f, 0.0f,
                             0.0f, 0.0f, 1.0f, 0.0f,
                             0.0f, 0.0f, 0.0f, 1.0f);


bool MatAffineInverse( const float src[4][4], float dst[4][4] )
{
    float temp[4][4];
    if (IsFloatZero(src[3][0]) && IsFloatZero(src[3][1]) && IsFloatZero(src[3][2]) && IsFloatZero(src[3][3] - 1.0f))
    {
        temp[0][0] = src[0][0]; temp[0][1] = src[1][0]; temp[0][2] = src[2][0];
        temp[1][0] = src[0][1]; temp[1][1] = src[1][1]; temp[1][2] = src[2][1];
        temp[2][0] = src[0][2]; temp[2][1] = src[1][2]; temp[2][2] = src[2][2];
        temp[3][0] = 0.0f;      temp[3][1] = 0.0f;      temp[3][2] = 0.0f;      temp[3][3] = 1.0f;

        float t[3]; t[0] = src[0][3]; t[1] = src[1][3]; t[2] = src[2][3];
        temp[0][3] = -VecDot(temp[0], t);
        temp[1][3] = -VecDot(temp[1], t);
        temp[2][3] = -VecDot(temp[2], t);
        memcpy(dst, temp, MATRIX44_SIZE);
        return true;
    }
    return false;
}


bool MatInverse(const float src[4][4], float dst[4][4])
{
    float temp[4][4];

    float a0 = src[0][0] * src[1][1] - src[1][0] * src[0][1];
    float a1 = src[0][0] * src[2][1] - src[2][0] * src[0][1];
    float a2 = src[0][0] * src[3][1] - src[3][0] * src[0][1];
    float a3 = src[1][0] * src[2][1] - src[2][0] * src[1][1];
    float a4 = src[1][0] * src[3][1] - src[3][0] * src[1][1];
    float a5 = src[2][0] * src[3][1] - src[3][0] * src[2][1];
    float b0 = src[0][2] * src[1][3] - src[1][2] * src[0][3];
    float b1 = src[0][2] * src[2][3] - src[2][2] * src[0][3];
    float b2 = src[0][2] * src[3][3] - src[3][2] * src[0][3];
    float b3 = src[1][2] * src[2][3] - src[2][2] * src[1][3];
    float b4 = src[1][2] * src[3][3] - src[3][2] * src[1][3];
    float b5 = src[2][2] * src[3][3] - src[3][2] * src[2][3];

    // Calculate the determinant.
    float det = a0 * b5 - a1 * b4 + a2 * b3 + a3 * b2 - a4 * b1 + a5 * b0;

    // Close to zero, can't invert.
    if (fabs(det) <= EPSILON)
        return false;

    // Support the case where m == dst.
    float inv_det = 1.0f / det;
    temp[0][0] = ( src[1][1] * b5 - src[2][1] * b4 + src[3][1] * b3) * inv_det;
    temp[1][0] = (-src[1][0] * b5 + src[2][0] * b4 - src[3][0] * b3) * inv_det;
    temp[2][0] = ( src[1][3] * a5 - src[2][3] * a4 + src[3][3] * a3) * inv_det;
    temp[3][0] = (-src[1][2] * a5 + src[2][2] * a4 - src[3][2] * a3) * inv_det;

    temp[0][1] = (-src[0][1] * b5 + src[2][1] * b2 - src[3][1] * b1) * inv_det;
    temp[1][1] = ( src[0][0] * b5 - src[2][0] * b2 + src[3][0] * b1) * inv_det;
    temp[2][1] = (-src[0][3] * a5 + src[2][3] * a2 - src[3][3] * a1) * inv_det;
    temp[3][1] = ( src[0][2] * a5 - src[2][2] * a2 + src[3][2] * a1) * inv_det;

    temp[0][2] = ( src[0][1] * b4 - src[1][1] * b2 + src[3][1] * b0) * inv_det;
    temp[1][2] = (-src[0][0] * b4 + src[1][0] * b2 - src[3][0] * b0) * inv_det;
    temp[2][2] = ( src[0][3] * a4 - src[1][3] * a2 + src[3][3] * a0) * inv_det;
    temp[3][2] = (-src[0][2] * a4 + src[1][2] * a2 - src[3][2] * a0) * inv_det;

    temp[0][3] = (-src[0][1] * b3 + src[1][1] * b1 - src[2][1] * b0) * inv_det;
    temp[1][3] = ( src[0][0] * b3 - src[1][0] * b1 + src[2][0] * b0) * inv_det;
    temp[2][3] = (-src[0][3] * a3 + src[1][3] * a1 - src[2][3] * a0) * inv_det;
    temp[3][3] = ( src[0][2] * a3 - src[1][2] * a1 + src[2][2] * a0) * inv_det;
    memcpy(dst, temp, MATRIX44_SIZE);
    
    return true;
}

void QuatNormalize( const float quatSrc[4], float quatDst[4] )
{
    float lenght = quatSrc[0] * quatSrc[0] + quatSrc[1] * quatSrc[1] +
                   quatSrc[2] * quatSrc[2] + quatSrc[3] * quatSrc[3];
    if(FloatEqual(lenght, 1.0))
    {
        quatDst[0] = quatSrc[0];
        quatDst[1] = quatSrc[1];
        quatDst[2] = quatSrc[2];
        quatDst[3] = quatSrc[3];
        return;
    }

    float lenghtRT = sqrtf(lenght);
    if(FloatEqual(lenghtRT, EPSILON) )
        return;

    float invLenghtRT = 1.0f / lenghtRT;
    quatDst[0] = quatSrc[0] * invLenghtRT;
    quatDst[1] = quatSrc[1] * invLenghtRT;
    quatDst[2] = quatSrc[2] * invLenghtRT;
    quatDst[3] = quatSrc[3] * invLenghtRT;
}

void QuatFromEulers( const float vec[3], float quat[4] )
{
    float angle;

    angle = vec[0] * 0.5f;
    const float sr = sin(angle);
    const float cr = cos(angle);

    angle = vec[1] * 0.5f;
    const float sp = sin(angle);
    const float cp = cos(angle);

    angle = vec[2] * 0.5f;
    const float sy = sin(angle);
    const float cy = cos(angle);

    const float cpcy = cp * cy;
    const float spcy = sp * cy;
    const float cpsy = cp * sy;
    const float spsy = sp * sy;

    quat[0] = (sr * cpcy - cr * spsy);
    quat[1] = (cr * spcy + sr * cpsy);
    quat[2] = (cr * cpsy - sr * spcy);
    quat[3] = (cr * cpcy + sr * spsy);
}

void QuatFromAxisAngle( const float axis[3], float angle, float quat[4] )
{
    float temp[3];
    VecCopy(axis, temp);
    VecNormalize(temp);
    float halfAngle = DEG2RAD(angle) * 0.5f;
    float sinHalfAngle = sinf(halfAngle);
    float cosHalfAngle = cosf(halfAngle);
    quat[0] = temp[0] * sinHalfAngle;
    quat[1] = temp[1] * sinHalfAngle;
    quat[2] = temp[2] * sinHalfAngle;
    quat[3] = cosHalfAngle;
}

void QuatFromTwoVector( const float start[3], const float end[3], float quat[4] )
{
    if (FloatEqualArray(&start[0], &end[0], 3))
    {
        quat[0] = 0.0f, quat[1] = 0.0f;
        quat[2] = 0.0f, quat[3] = 1.0f;
        return;
    }

    float normStart[3];
    float normEnd[3];
    VecCopy(start, normStart);
    VecNormalize(normStart);
    VecCopy(end, normEnd);
    VecNormalize(normEnd);
    float d = VecDot(normStart, normEnd);

    if (d > -1.0f + EPSILON)
    {
        float c[3];
        VecCross(normStart, normEnd, c);
        float s = sqrtf((1.0f + d) * 2.0f);
        float invS = 1.0f / s;

        quat[0] = c[0] * invS;
        quat[1] = c[1] * invS;
        quat[2] = c[2] * invS;
        quat[3] = 0.5f * s;
    }
    else
    {
        float axis[3];
        VecCross(VectorRight, normStart, axis);
        if (VecLength(axis) < EPSILON)
           VecCross(VectorUp, normStart, axis);

        QuatFromAxisAngle(axis, 180.0f, quat);
    }
}

void QuatMultiply( const float quat1[4], const float quat2[4], float result[4] )
{
    result[0] = quat1[3] * quat2[0] + quat1[0] * quat2[3] + quat1[1] * quat2[2] - quat1[2] * quat2[1];
    result[1] = quat1[3] * quat2[1] - quat1[0] * quat2[2] + quat1[1] * quat2[3] + quat1[2] * quat2[0];
    result[2] = quat1[3] * quat2[2] + quat1[0] * quat2[1] - quat1[1] * quat2[0] + quat1[2] * quat2[3];
    result[3] = quat1[3] * quat2[3] - quat1[0] * quat2[0] - quat1[1] * quat2[1] - quat1[2] * quat2[2];
}

void QuatToMat( const float quat[4], float matrix[4][4] )
{
    matrix[0][0] = 1.0f - 2.0f * quat[1] * quat[1] - 2.0f * quat[2] * quat[2];
    matrix[1][0] = 2.0f * quat[0] * quat[1] + 2.0f * quat[3] * quat[2];
    matrix[2][0] = 2.0f * quat[0] * quat[2] - 2.0f * quat[3] * quat[1];

    matrix[0][1] = 2.0f * quat[0] * quat[1] - 2.0f * quat[3] * quat[2];
    matrix[1][1] = 1.0f - 2.0f * quat[0] * quat[0] - 2.0f * quat[2] * quat[2];
    matrix[2][1] = 2.0f * quat[1] * quat[2] + 2.0f * quat[3] * quat[0];

    matrix[0][2] = 2.0f * quat[0] * quat[2] + 2.0f * quat[3] * quat[1];
    matrix[1][2] = 2.0f * quat[1] * quat[2] - 2.0f * quat[3] * quat[0];
    matrix[2][2] = 1.0f - 2.0f * quat[0] * quat[0] - 2.0f * quat[1] * quat[1];

    matrix[3][0] = 0.0f;
    matrix[3][1] = 0.0f;
    matrix[3][2] = 0.0f;

    matrix[0][3] = 0.0f;
    matrix[1][3] = 0.0f;
    matrix[2][3] = 0.0f;

    matrix[3][3] = 1.0f;
}

void QuatSlerp( const float quat1[4], const float quat2[4], float interpTime, float result[4] )
{
    AASSERT(interpTime >= 0.0f && interpTime <= 1.0f && "interpTime must be between [0 - 1] space.");
    if(FloatEqual(interpTime, 0.0f))
    {
        result[0] = quat1[0]; 
        result[1] = quat1[1];
        result[2] = quat1[2];
        result[3] = quat1[3];
        return;
    }
    else if(FloatEqual(interpTime, 1.0f))
    {
        result[0] = quat2[0]; 
        result[1] = quat2[1];
        result[2] = quat2[2];
        result[3] = quat2[3];
        return;
    }
    if (FloatEqual(quat1[0],quat2[0]) && FloatEqual(quat1[1],quat2[1]) &&
        FloatEqual(quat1[2],quat2[2]) && FloatEqual(quat1[3],quat2[3]) )
    {
        result[0] = quat1[0]; 
        result[1] = quat1[1];
        result[2] = quat1[2];
        result[3] = quat1[3];
        return;
    }

    float cosOmega = Vec4Dot(quat1, quat2);
    float temp[4];
    // make sure we use the short rotation
    if(cosOmega < 0.0f)
    {
        temp[0] = -quat2[0];
        temp[1] = -quat2[1];
        temp[2] = -quat2[2];
        temp[3] = -quat2[3];
        cosOmega = -cosOmega;
    }else
    {
        temp[0] = quat2[0];
        temp[1] = quat2[1];
        temp[2] = quat2[2];
        temp[3] = quat2[3];
    }

    float k0, k1;
    if(cosOmega < 0.985f)
    {
        // spherical interpolation
        //const float theta = acosf(cosOmega);
        //const float invSinTheta = 1.0f /sinf(theta);
        //k0 = sinf(theta * (1.0f - interpTime)) * invSinTheta;
        //k1 = sinf(theta * interpTime) * invSinTheta;
        float sinOmega = sqrtf(1.0f - cosOmega * cosOmega);
        float omega = atan2f(sinOmega, cosOmega);
        float oneOverSinOmega = 1.0f / sinOmega;
        k0 = sinf((1.0f - interpTime) * omega) * oneOverSinOmega;
        k1 = sinf(interpTime * omega) * oneOverSinOmega;
    }
    else
    {
        // linear interploation
        k0 = 1.0f - interpTime;
        k1 = interpTime;
    }
    result[0] = quat1[0] * k0 + temp[0] * k1;
    result[1] = quat1[1] * k0 + temp[1] * k1;
    result[2] = quat1[2] * k0 + temp[2] * k1;
    result[3] = quat1[3] * k0 + temp[3] * k1;

    // half-life / Valve
    //int i;
    //float omega, cosom, sinom, sclp, sclq;

    //// decide if one of the quaternions is backwards
    //float a = 0;
    //float b = 0;
    //for (i = 0; i < 4; i++) {
    //    a += (quat1[i]-quat2[i])*(quat1[i]-quat2[i]);
    //    b += (quat1[i]+quat2[i])*(quat1[i]+quat2[i]);
    //}
    //float temp[4];
    //if (a > b) {
    //    for (i = 0; i < 4; i++) {
    //        temp[i] = -quat2[i];
    //    }
    //}else
    //{
    //    temp[0] = quat2[0];
    //    temp[1] = quat2[1];
    //    temp[2] = quat2[2];
    //    temp[3] = quat2[3];
    //}

    //cosom = quat1[0]*temp[0] + quat1[1]*temp[1] + quat1[2]*temp[2] + quat1[3]*temp[3];

    //if ((1.0 + cosom) > 0.00000001) {
    //    if ((1.0 - cosom) > 0.00000001) {
    //        omega = acos( cosom );
    //        sinom = sin( omega );
    //        sclp = sin( (1.0 - interpTime)*omega) / sinom;
    //        sclq = sin( interpTime*omega ) / sinom;
    //    }
    //    else {
    //        sclp = 1.0 - interpTime;
    //        sclq = interpTime;
    //    }
    //    for (i = 0; i < 4; i++) {
    //        result[i] = sclp * quat1[i] + sclq * temp[i];
    //    }
    //}
    //else {
    //    result[0] = -quat1[1];
    //    result[1] = quat1[0];
    //    result[2] = -quat1[3];
    //    result[3] = quat1[2];
    //    sclp = sin( (1.0 - interpTime) * 0.5 * MATH_PI);
    //    sclq = sin( interpTime * 0.5 * MATH_PI);
    //    for (i = 0; i < 3; i++) {
    //        result[i] = sclp * quat1[i] + sclq * result[i];
    //    }
    //}
}

void SlerpForSquad( const float quat1[4], const float quat2[4], float interpTime, float result[4])
{
    AASSERT(result);

    // cos(omega) = q1 * q2;
    // slerp(q1, q2, t) = (q1*sin((1-t)*omega) + q2*sin(t*omega))/sin(omega);
    // q1 = +- q2, slerp(q1,q2,t) = q1.
    // This is a straight-forward implementation of the formula of slerp. It does not do any sign switching.
    float c = Vec4Dot(quat1, quat2);

    if (fabs(c) >= 1.0f)
    {
        result[0] = quat2[0];
        result[1] = quat2[1];
        result[2] = quat2[2];
        result[3] = quat2[3];
        return;
    }

    float omega = acos(c);
    float s = sqrt(1.0f - c * c);
    if (fabs(s) <= 0.00001f)
    {
        result[0] = quat1[0];
        result[1] = quat1[1];
        result[2] = quat1[2];
        result[3] = quat1[3];
        return;
    }

    float r1 = sin((1 - interpTime) * omega) / s;
    float r2 = sin(interpTime * omega) / s;
    result[0] = (quat1[0] * r1 + quat2[0] * r2);
    result[1] = (quat1[1] * r1 + quat2[1] * r2);
    result[2] = (quat1[2] * r1 + quat2[2] * r2);
    result[3] = (quat1[3] * r1 + quat2[3] * r2);
}

void QuatSquad( const float quat1[4], const float quat2[4], const float ctrl1[4], const float ctrl2[4], float interpTime, float result[4] )
{
    AASSERT(interpTime >= 0.0f && interpTime <= 1.0f && "interpTime must be between [0 - 1] space.");

    float dstQ[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    float dstS[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

    SlerpForSquad(quat1, quat2, interpTime, dstQ);
    SlerpForSquad(ctrl1, ctrl2, interpTime, dstS);
    SlerpForSquad(dstQ, dstS, 2.0f * interpTime * (1.0f - interpTime), result);
}

void MatToQuat( const float mat[4][4], float quat[4] )
{
    float temp[4];
    const float diag = mat[0][0] + mat[1][1] + mat[2][2] + 1;

    if( diag > 0.0f )
    {
        const float scale = sqrtf(diag) * 2.0f; // get scale from diagonal

        // TODO: speed this up
        temp[0] = (mat[2][1] - mat[1][2]) / scale;
        temp[1] = (mat[0][2] - mat[2][0]) / scale;
        temp[2] = (mat[1][0] - mat[0][1]) / scale;
        temp[3] = 0.25f * scale;
    }
    else
    {
        if (mat[0][0] > mat[1][1] && mat[0][0] > mat[2][2])
        {
            // 1st element of diag is greatest value
            // find scale according to 1st element, and double it
            const float scale = sqrtf(1.0f + mat[0][0] - mat[1][1] - mat[2][2]) * 2.0f;

            // TODO: speed this up
            temp[0] = 0.25f * scale;
            temp[1] = (mat[0][1] + mat[1][0]) / scale;
            temp[2] = (mat[2][0] + mat[0][2]) / scale;
            temp[3] = (mat[2][1] - mat[1][2]) / scale;
        }
        else if (mat[1][1] > mat[2][2])
        {
            // 2nd element of diag is greatest value
            // find scale according to 2nd element, and double it
            const float scale = sqrtf(1.0f + mat[1][1] - mat[0][0] - mat[2][2]) * 2.0f;

            // TODO: speed this up
            temp[0] = (mat[0][1] + mat[1][0]) / scale;
            temp[1] = 0.25f * scale;
            temp[2] = (mat[1][2] + mat[2][1]) / scale;
            temp[3] = (mat[0][2] - mat[2][0]) / scale;
        }
        else
        {
            // 3rd element of diag is greatest value
            // find scale according to 3rd element, and double it
            const float scale = sqrtf(1.0f + mat[2][2] - mat[0][0] - mat[1][1]) * 2.0f;

            // TODO: speed this up
            temp[0] = (mat[0][2] + mat[2][0]) / scale;
            temp[1] = (mat[1][2] + mat[2][1]) / scale;
            temp[2] = 0.25f * scale;
            temp[3] = (mat[1][0] - mat[0][1]) / scale;
        }
    }

    return QuatNormalize(temp, quat);
}

void ConcatTransforms( const float mat1[4][4], const float mat2[4][4],float result[4][4] )
{
    float temp[4][4];
    temp[0][0] = mat1[0][0] * mat2[0][0] + mat1[0][1] * mat2[1][0] +
                 mat1[0][2] * mat2[2][0] + mat1[0][3] * mat2[3][0];
    temp[0][1] = mat1[0][0] * mat2[0][1] + mat1[0][1] * mat2[1][1] +
                 mat1[0][2] * mat2[2][1] + mat1[0][3] * mat2[3][1];
    temp[0][2] = mat1[0][0] * mat2[0][2] + mat1[0][1] * mat2[1][2] +
                 mat1[0][2] * mat2[2][2] + mat1[0][3] * mat2[3][2];
    temp[0][3] = mat1[0][0] * mat2[0][3] + mat1[0][1] * mat2[1][3] +
                 mat1[0][2] * mat2[2][3] + mat1[0][3] * mat2[3][3];
    temp[1][0] = mat1[1][0] * mat2[0][0] + mat1[1][1] * mat2[1][0] +
                 mat1[1][2] * mat2[2][0] + mat1[1][3] * mat2[3][0];
    temp[1][1] = mat1[1][0] * mat2[0][1] + mat1[1][1] * mat2[1][1] +
                 mat1[1][2] * mat2[2][1] + mat1[1][3] * mat2[3][1];
    temp[1][2] = mat1[1][0] * mat2[0][2] + mat1[1][1] * mat2[1][2] +
                 mat1[1][2] * mat2[2][2] + mat1[1][3] * mat2[3][2];
    temp[1][3] = mat1[1][0] * mat2[0][3] + mat1[1][1] * mat2[1][3] +
                 mat1[1][2] * mat2[2][3] + mat1[1][3] * mat2[3][3];
    temp[2][0] = mat1[2][0] * mat2[0][0] + mat1[2][1] * mat2[1][0] +
                 mat1[2][2] * mat2[2][0] + mat1[2][3] * mat2[3][0];
    temp[2][1] = mat1[2][0] * mat2[0][1] + mat1[2][1] * mat2[1][1] +
                 mat1[2][2] * mat2[2][1] + mat1[2][3] * mat2[3][1];
    temp[2][2] = mat1[2][0] * mat2[0][2] + mat1[2][1] * mat2[1][2] +
                 mat1[2][2] * mat2[2][2] + mat1[2][3] * mat2[3][2];
    temp[2][3] = mat1[2][0] * mat2[0][3] + mat1[2][1] * mat2[1][3] +
                 mat1[2][2] * mat2[2][3] + mat1[2][3] * mat2[3][3];
    temp[3][0] = mat1[3][0] * mat2[0][0] + mat1[3][1] * mat2[1][0] +
                 mat1[3][2] * mat2[2][0] + mat1[3][3] * mat2[3][0];
    temp[3][1] = mat1[3][0] * mat2[0][1] + mat1[3][1] * mat2[1][1] +
                 mat1[3][2] * mat2[2][1] + mat1[3][3] * mat2[3][1];
    temp[3][2] = mat1[3][0] * mat2[0][2] + mat1[3][1] * mat2[1][2] +
                 mat1[3][2] * mat2[2][2] + mat1[3][3] * mat2[3][2];
    temp[3][3] = mat1[3][0] * mat2[0][3] + mat1[3][1] * mat2[1][3] +
                 mat1[3][2] * mat2[2][3] + mat1[3][3] * mat2[3][3];
    memcpy(result, temp, MATRIX44_SIZE);
}

void VecRotate( const float vec[3], const float quat[4], float result[3] )
{
    if(0)
    {
        // normal method
        float q1[4], q2[4], q3[4];
        q1[0] = quat[0]; q1[1] = quat[1]; q1[2] = quat[2]; q1[3] = quat[3];
        q2[0] =  vec[0]; q2[1] =  vec[1]; q2[2] =  vec[2]; q2[3] = 0.0f;
        q3[0] =  -q1[0]; q3[1] =  -q1[0]; q3[2] =  -q1[2]; q3[3] = q1[3];

        float temp[4];
        QuatMultiply(q1, q2, temp);
        QuatMultiply(temp, q3, q1);
        result[0] = q1[0];
        result[1] = q1[1];
        result[2] = q1[2];
        return;
    }

    // nVidia SDK implementation

    float uv[3];
    float uuv[3];
    float qvec[3];
    qvec[0] = quat[0];
    qvec[1] = quat[1];
    qvec[2] = quat[2];

    VecCross(qvec, vec, uv);
    VecCross(qvec, uv, uuv);

    float halfW = 2.0f * quat[3];

    result[0] = vec[0] + uv[0] * halfW + uuv[0] * 2.0f;
    result[1] = vec[1] + uv[1] * halfW + uuv[1] * 2.0f;
    result[2] = vec[2] + uv[2] * halfW + uuv[2] * 2.0f;
}
