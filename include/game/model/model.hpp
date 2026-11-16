#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <math.h>
#include "../../driver/driver_context.hpp"

#define HIDWORD(x) ((uint32_t)(((uint64_t)(x) >> 32) & 0xFFFFFFFF))

template<class T>  int16_t __PAIR__(int8_t  high, T low) { return (((int16_t)high) << sizeof(high) * 8) | uint8_t(low); }
template<class T>  int32_t __PAIR__(int16_t high, T low) { return (((int32_t)high) << sizeof(high) * 8) | uint16_t(low); }
template<class T>  int64_t __PAIR__(int32_t high, T low) { return (((int64_t)high) << sizeof(high) * 8) | uint32_t(low); }
template<class T> uint16_t __PAIR__(uint8_t  high, T low) { return (((uint16_t)high) << sizeof(high) * 8) | uint8_t(low); }
template<class T> uint32_t __PAIR__(uint16_t high, T low) { return (((uint32_t)high) << sizeof(high) * 8) | uint16_t(low); }
template<class T> uint64_t __PAIR__(uint32_t high, T low) { return (((uint64_t)high) << sizeof(high) * 8) | uint32_t(low); }

template<class T> T __ROL__(T value, int count)
{
    const std::uint32_t nbits = sizeof(T) * 8;

    if (count > 0)
    {
        count %= nbits;
        T high = value >> (nbits - count);
        if (T(-1) < 0)
            high &= ~((T(-1) << count));
        value <<= count;
        value |= high;
    }
    else
    {
        count = -count % nbits;
        T low = value << (nbits - count);
        value >>= count;
        value |= low;
    }
    return value;
}

inline std::uint8_t  __ROL1__(std::uint8_t  value, int count) { return __ROL__((std::uint8_t)value, count); }
inline std::uint16_t __ROL2__(std::uint16_t value, int count) { return __ROL__((std::uint16_t)value, count); }
inline std::uint32_t __ROL4__(std::uint32_t value, int count) { return __ROL__((std::uint32_t)value, count); }
inline std::uint64_t __ROL8__(std::uint64_t value, int count) { return __ROL__((std::uint64_t)value, count); }
inline std::uint8_t  __ROR1__(std::uint8_t  value, int count) { return __ROL__((std::uint8_t)value, -count); }
inline std::uint16_t __ROR2__(std::uint16_t value, int count) { return __ROL__((std::uint16_t)value, -count); }
inline std::uint32_t __ROR4__(std::uint32_t value, int count) { return __ROL__((std::uint32_t)value, -count); }
inline std::uint64_t __ROR8__(std::uint64_t value, int count) { return __ROL__((std::uint64_t)value, -count); }

namespace maxi::game::model {


    class UObject;
    class UClass;
    class UWorld;
    class ULevel;
    class AActor;
    class APlayerController;
    class APawn;
    class ACharacter;

    struct FUObjectItem
    {

        uintptr_t Object;

        int32_t Flags;

        int32_t ClusterRootIndex;

        int32_t SerialNumber;
        uintptr_t StatID;
    };

    struct FChunkedFixedUObjectArray
    {
        
        FUObjectItem** Objects;
        
        FUObjectItem* PreAllocatedObjects;
        
        int32_t MaxElements;
        
        int32_t NumElements;
        
        int32_t MaxChunks;
        
        int32_t NumChunks;
    };

    typedef FChunkedFixedUObjectArray TypeUObjectArray;

    


    struct FPointer {
        uintptr_t Dummy;
    };

    


    struct FVector {
        double X, Y, Z;

        FVector() : X(0.0), Y(0.0), Z(0.0) {}

        FVector(double X, double Y, double Z) : X(X), Y(Y), Z(Z) {}

        FVector(double InF) : X(InF), Y(InF), Z(InF) {}

        FVector operator+(const FVector& other) const { return FVector(X + other.X, Y + other.Y, Z + other.Z); }

        FVector operator-(const FVector& other) const { return FVector(X - other.X, Y - other.Y, Z - other.Z); }

        FVector operator*(const FVector& V) const { return FVector(X * V.X, Y * V.Y, Z * V.Z); }

        FVector operator/(const FVector& V) const { return FVector(X / V.X, Y / V.Y, Z / V.Z); }

        bool operator==(const FVector& V) const { return X == V.X && Y == V.Y && Z == V.Z; }

        bool operator!=(const FVector& V) const { return X != V.X || Y != V.Y || Z != V.Z; }

        FVector operator-() const { return FVector(-X, -Y, -Z); }

        FVector operator+(double Bias) const { return FVector(X + Bias, Y + Bias, Z + Bias); }

        FVector operator-(double Bias) const { return FVector(X - Bias, Y - Bias, Z - Bias); }

        FVector operator*(double Scale) const { return FVector(X * Scale, Y * Scale, Z * Scale); }

        FVector operator/(double Scale) const { const double RScale = 1.0 / Scale; return FVector(X * RScale, Y * RScale, Z * RScale); }

        FVector operator=(const FVector& V) { X = V.X; Y = V.Y; Z = V.Z; return *this; }

        FVector operator+=(const FVector& V) { X += V.X; Y += V.Y; Z += V.Z; return *this; }

        FVector operator-=(const FVector& V) { X -= V.X; Y -= V.Y; Z -= V.Z; return *this; }

        FVector operator*=(const FVector& V) { X *= V.X; Y *= V.Y; Z *= V.Z; return *this; }

        FVector operator/=(const FVector& V) { X /= V.X; Y /= V.Y; Z /= V.Z; return *this; }

        FVector operator*=(double Scale) { X *= Scale; Y *= Scale; Z *= Scale; return *this; }

        FVector operator/=(double V) { const double RV = 1.0 / V; X *= RV; Y *= RV; Z *= RV; return *this; }

        double operator|(const FVector& V) const { return X * V.X + Y * V.Y + Z * V.Z; }

        FVector operator^(const FVector& V) const { return FVector(Y * V.Z - Z * V.Y, Z * V.X - X * V.Z, X * V.Y - Y * V.X); }

        __forceinline double SiZeSquared() const {
            return X * X + Y * Y + Z * Z;
        }

        __forceinline double SizeSquared2D() const {
            return X * X + Y * Y;
        }

        __forceinline double Magnitude() const
        {
            return ::sqrt(X * X + Y * Y + Z * Z);
        }

        __forceinline double Dot(const FVector& vOther) const {
            const FVector& a = *this;

            return (a.X * vOther.X + a.Y * vOther.Y + a.Z * vOther.Z);
        }

        double Distance(FVector v)
        {
            double dx = (double)(this->X - v.X);
            double dy = (double)(this->Y - v.Y);
            double dz = (double)(this->Z - v.Z);
            double distance_units = sqrt((dx * dx) + (dy * dy) + (dz * dz));
            return distance_units;
        }

        FVector Normalize()
        {
            FVector vector;
            double length = this->Magnitude();

            if (length != 0)
            {
                vector.X = X / length;
                vector.Y = Y / length;
                vector.Z = Z / length;
            }
            else
            {
                vector.X = vector.Y = 0.0;
                vector.Z = 1.0;
            }
            return vector;
        }

        inline double Dot(FVector v)
        {
            return X * v.X + Y * v.Y + Z * v.Z;
        }
    };

    


    struct FVector2D {
        double                                             X;
        double                                             Y;

        inline FVector2D()
            : X(0), Y(0)
        {
        }

        inline FVector2D(double x, double y)
            : X(x),
            Y(y)
        {
        }

        __forceinline FVector2D operator-(const FVector2D& V) {
            return FVector2D(X - V.X, Y - V.Y);
        }

        __forceinline FVector2D operator+(const FVector2D& V) {
            return FVector2D(X + V.X, Y + V.Y);
        }

        __forceinline FVector2D operator*(double Scale) const {
            return FVector2D(X * Scale, Y * Scale);
        }

        __forceinline FVector2D operator/(double Scale) const {
            const double RScale = 1.0 / Scale;
            return FVector2D(X * RScale, Y * RScale);
        }

        __forceinline FVector2D operator+(double A) const {
            return FVector2D(X + A, Y + A);
        }

        __forceinline FVector2D operator-(double A) const {
            return FVector2D(X - A, Y - A);
        }

        __forceinline FVector2D operator*(const FVector2D& V) const {
            return FVector2D(X * V.X, Y * V.Y);
        }

        __forceinline FVector2D operator/(const FVector2D& V) const {
            return FVector2D(X / V.X, Y / V.Y);
        }

        __forceinline double operator|(const FVector2D& V) const {
            return X * V.X + Y * V.Y;
        }

        __forceinline double operator^(const FVector2D& V) const {
            return X * V.Y - Y * V.X;
        }

        __forceinline FVector2D& operator+=(const FVector2D& v) {
            X += v.X;
            Y += v.Y;
            return *this;
        }

        __forceinline FVector2D& operator-=(const FVector2D& v) {
            X -= v.X;
            Y -= v.Y;
            return *this;
        }

        __forceinline FVector2D& operator*=(const FVector2D& v) {
            X *= v.X;
            Y *= v.Y;
            return *this;
        }

        __forceinline FVector2D& operator/=(const FVector2D& v) {
            X /= v.X;
            Y /= v.Y;
            return *this;
        }

        friend FVector2D operator*(double scalar, const FVector2D& V) {
            return FVector2D(scalar * V.X, scalar * V.Y);
        }

        __forceinline bool operator==(const FVector2D& src) const {
            return (src.X == X) && (src.Y == Y);
        }

        __forceinline bool operator!=(const FVector2D& src) const {
            return (src.X != X) || (src.Y != Y);
        }

        __forceinline double SizeSquared() const {
            return X * X + Y * Y;
        }

        __forceinline double Dot(const FVector2D& vOther) const {
            const FVector2D& a = *this;

            return (a.X * vOther.X + a.Y * vOther.Y);
        }

        bool isValid() const {
            return !isnan(X) && !isnan(Y) && !isinf(X) && !isinf(Y) && (X > 0 || Y > 0);
        }
	};

    


    struct FRotator {
        double Pitch;
        double Yaw;
        double Roll;

        inline FRotator() : Pitch(0), Yaw(0), Roll(0) {}

        inline FRotator(double x, double y, double z) : Pitch(x), Yaw(y), Roll(z) {}

        FRotator operator+(const FRotator& V) {
            return FRotator(Pitch + V.Pitch, Yaw + V.Yaw, Roll + V.Roll);
        }

        FRotator operator-(const FRotator& V) {
            return FRotator(Pitch - V.Pitch, Yaw - V.Yaw, Roll - V.Roll);
        }

        FRotator operator*(double Scale) const {
            return FRotator(Pitch * Scale, Yaw * Scale, Roll * Scale);
        }

        FRotator operator/(double Scale) const {
            const double RScale = 1.0 / Scale;
            return FRotator(Pitch * RScale, Yaw * RScale, Roll * RScale);
        }

        FRotator operator+(double A) const {
            return FRotator(Pitch + A, Yaw + A, Roll + A);
        }

        FRotator operator+(FRotator A) const {
            return FRotator(Pitch + A.Pitch, Yaw + A.Yaw, Roll + A.Roll);
        }

        FRotator operator-(double A) const {
            return FRotator(Pitch - A, Yaw - A, Roll - A);
        }

        FRotator operator-(FRotator A) const {
            return FRotator(Pitch - A.Pitch, Yaw - A.Yaw, Roll - A.Roll);
        }

        FRotator operator*(const FRotator& V) const {
            return FRotator(Pitch * V.Pitch, Yaw * V.Yaw, Roll * V.Roll);
        }

        FRotator operator/(const FRotator& V) const {
            return FRotator(Pitch / V.Pitch, Yaw / V.Yaw, Roll / V.Roll);
        }

        double operator|(const FRotator& V) const {
            return Pitch * V.Pitch + Yaw * V.Yaw + Roll * V.Roll;
        }

        FRotator& operator+=(const FRotator& v) {
            Pitch += v.Pitch;
            Yaw += v.Yaw;
            Roll += v.Roll;
            return *this;
        }

        FRotator& operator-=(const FRotator& v) {
            Pitch -= v.Pitch;
            Yaw -= v.Yaw;
            Roll -= v.Roll;
            return *this;
        }

        FRotator& operator*=(const FRotator& v) {
            Pitch *= v.Pitch;
            Yaw *= v.Yaw;
            Roll *= v.Roll;
            return *this;
        }

        FRotator& operator/=(const FRotator& v) {
            Pitch /= v.Pitch;
            Yaw /= v.Yaw;
            Roll /= v.Roll;
            return *this;
        }

        double operator^(const FRotator& V) const {
            return Pitch * V.Yaw - Yaw * V.Pitch - Roll * V.Roll;
        }

        bool operator==(const FRotator& src) const {
            return (src.Pitch == Pitch) && (src.Yaw == Yaw) && (src.Roll == Roll);
        }

        bool operator!=(const FRotator& src) const {
            return (src.Pitch != Pitch) || (src.Yaw != Yaw) || (src.Roll != Roll);
        }

        double Size() const {
            return ::sqrt(Pitch * Pitch + Yaw * Yaw + Roll * Roll);
        }

        double SizeSquared() const {
            return Pitch * Pitch + Yaw * Yaw + Roll * Roll;
        }

        double Dot(const FRotator& vOther) const {
            const FRotator& a = *this;
            return (a.Pitch * vOther.Pitch + a.Yaw * vOther.Yaw + a.Roll * vOther.Roll);
        }

        double ClampAxis(double Angle) {

            Angle = fmod(Angle, 360.0);

            if (Angle < 0.0) {
                Angle += 360.0;
            }

            return Angle;
        }

        double NormalizeAxis(double Angle) {

            Angle = ClampAxis(Angle);

            if (Angle > 180.0) {
                Angle -= 360.0;
            }

            return Angle;
        }

        void NormalizeAngles()
        {
            if (Pitch > 89.0) Pitch -= 360.0;
            else if (Pitch < -89.0) Pitch += 360.0;

            while (Yaw > 180.0)Yaw -= 360.0;
            while (Yaw < -180.0)Yaw += 360.0;
            Roll = 0;
        }

        void Normalize() {
            Pitch = NormalizeAxis(Pitch);
            Yaw = NormalizeAxis(Yaw);
            Roll = NormalizeAxis(Roll);
        }

        FRotator GetNormalized() const {
            FRotator Rot = *this;
            Rot.Normalize();
            return Rot;
        }

        FVector ToVector() const {
            return FVector(Pitch, Yaw, Roll);
        }
    };

    


    struct alignas(16) FPlane : public FVector {
        double W;
    };

    


    struct FQuat {
        double X;
        double Y;
        double Z;
        double W;

        FQuat() : X(0.0), Y(0.0), Z(0.0), W(1.0) {}
        FQuat(double x, double y, double z, double w) : X(x), Y(y), Z(z), W(w) {}
    };

    struct FMatrix
    {
        FPlane                                      XPlane;
        FPlane                                      YPlane;
        FPlane                                      ZPlane;
        FPlane                                      WPlane;

        FMatrix operator*(const FMatrix& pM2)
        {
            FMatrix pOut;
            pOut.XPlane.X = XPlane.X * pM2.XPlane.X + XPlane.Y * pM2.YPlane.X + XPlane.Z * pM2.ZPlane.X + XPlane.W * pM2.WPlane.X;
            pOut.XPlane.Y = XPlane.X * pM2.XPlane.Y + XPlane.Y * pM2.YPlane.Y + XPlane.Z * pM2.ZPlane.Y + XPlane.W * pM2.WPlane.Y;
            pOut.XPlane.Z = XPlane.X * pM2.XPlane.Z + XPlane.Y * pM2.ZPlane.Z + XPlane.Z * pM2.ZPlane.Z + XPlane.W * pM2.WPlane.Z;
            pOut.XPlane.W = XPlane.X * pM2.XPlane.W + XPlane.Y * pM2.YPlane.W + XPlane.Z * pM2.ZPlane.W + XPlane.W * pM2.WPlane.W;
            pOut.YPlane.X = YPlane.X * pM2.XPlane.X + YPlane.Y * pM2.YPlane.X + YPlane.Z * pM2.ZPlane.X + YPlane.W * pM2.WPlane.X;
            pOut.YPlane.Y = YPlane.X * pM2.XPlane.Y + YPlane.Y * pM2.YPlane.Y + YPlane.Z * pM2.ZPlane.Y + YPlane.W * pM2.WPlane.Y;
            pOut.YPlane.Z = YPlane.X * pM2.XPlane.Z + YPlane.Y * pM2.ZPlane.Z + YPlane.Z * pM2.ZPlane.Z + YPlane.W * pM2.WPlane.Z;
            pOut.YPlane.W = YPlane.X * pM2.XPlane.W + YPlane.Y * pM2.YPlane.W + YPlane.Z * pM2.ZPlane.W + YPlane.W * pM2.WPlane.W;
            pOut.ZPlane.X = ZPlane.X * pM2.XPlane.X + ZPlane.Y * pM2.YPlane.X + ZPlane.Z * pM2.ZPlane.X + ZPlane.W * pM2.WPlane.X;
            pOut.ZPlane.Y = ZPlane.X * pM2.XPlane.Y + ZPlane.Y * pM2.YPlane.Y + ZPlane.Z * pM2.ZPlane.Y + ZPlane.W * pM2.WPlane.Y;
            pOut.ZPlane.Z = ZPlane.X * pM2.XPlane.Z + ZPlane.Y * pM2.ZPlane.Z + ZPlane.Z * pM2.ZPlane.Z + ZPlane.W * pM2.WPlane.Z;
            pOut.ZPlane.W = ZPlane.X * pM2.XPlane.W + ZPlane.Y * pM2.YPlane.W + ZPlane.Z * pM2.ZPlane.W + ZPlane.W * pM2.WPlane.W;
            pOut.WPlane.X = WPlane.X * pM2.XPlane.X + WPlane.Y * pM2.YPlane.X + WPlane.Z * pM2.ZPlane.X + WPlane.W * pM2.WPlane.X;
            pOut.WPlane.Y = WPlane.X * pM2.XPlane.Y + WPlane.Y * pM2.YPlane.Y + WPlane.Z * pM2.ZPlane.Y + WPlane.W * pM2.WPlane.Y;
            pOut.WPlane.Z = WPlane.X * pM2.XPlane.Z + WPlane.Y * pM2.ZPlane.Z + WPlane.Z * pM2.ZPlane.Z + WPlane.W * pM2.WPlane.Z;
            pOut.WPlane.W = WPlane.X * pM2.XPlane.W + WPlane.Y * pM2.YPlane.W + WPlane.Z * pM2.ZPlane.W + WPlane.W * pM2.WPlane.W;
            return pOut;
        }
    };

    


    struct FTransform {
        struct FQuat                                       Rotation;
        struct FVector                                     Translation;
        unsigned char                                      UnknownData00[0x4];
        struct FVector                                     Scale3D;
        unsigned char                                      UnknownData01[0x4];

        FMatrix ToMatrixWithScale() const
        {
            FMatrix m;
            m.WPlane.X = Translation.X;
            m.WPlane.Y = Translation.Y;
            m.WPlane.Z = Translation.Z;

            double x2 = Rotation.X + Rotation.X;
            double y2 = Rotation.Y + Rotation.Y;
            double z2 = Rotation.Z + Rotation.Z;

            double xx2 = Rotation.X * x2;
            double yy2 = Rotation.Y * y2;
            double zz2 = Rotation.Z * z2;
            m.XPlane.X = (1.0 - (yy2 + zz2)) * Scale3D.X;
            m.YPlane.Y = (1.0 - (xx2 + zz2)) * Scale3D.X;
            m.ZPlane.Z = (1.0 - (xx2 + yy2)) * Scale3D.X;

            double yz2 = Rotation.Y * z2;
            double wx2 = Rotation.W * x2;
            m.ZPlane.Y = (yz2 - wx2) * Scale3D.Z;
            m.YPlane.Z = (yz2 + wx2) * Scale3D.Y;

            double xy2 = Rotation.X * y2;
            double wz2 = Rotation.W * z2;
            m.YPlane.X = (xy2 - wz2) * Scale3D.Y;
            m.XPlane.Y = (xy2 + wz2) * Scale3D.X;

            double xz2 = Rotation.X * z2;
            double wy2 = Rotation.W * y2;
            m.ZPlane.X = (xz2 + wy2) * Scale3D.Z;
            m.XPlane.Z = (xz2 - wy2) * Scale3D.X;

            m.XPlane.W = 0.0;
            m.YPlane.W = 0.0;
            m.ZPlane.W = 0.0;
            m.WPlane.W = 1.0;

            return m;
        }
    };

    


    struct D3DMatrix {
        union {
            struct {
                double        _11, _12, _13, _14;
                double        _21, _22, _23, _24;
                double        _31, _32, _33, _34;
                double        _41, _42, _43, _44;

            };
            double m[4][4];
        };

        D3DMatrix() {
            memset(m, 0, sizeof(m));
		}
    };

    struct FMinimalViewInfo
    {
        FVector                                            Location;
        FRotator                                           Rotation;
        float                                              FOV;
        float                                              DesiredFOV;
        float                                              OrthoWidth;
        float                                              OrthoNearClipPlane;
        float                                              OrthoFarClipPlane;
        float                                              PerspectiveNearClipPlane;
        float                                              AspectRatio;
    };



    static_assert(sizeof(FVector) == 24, "FVector must be 3x double (UE5 LWC)");
    static_assert(sizeof(FVector2D) == 16, "FVector2D must be 2x double");
    static_assert(sizeof(FRotator) == 24, "FRotator must be 3x double");
    static_assert(sizeof(FQuat) == 32, "FQuat must be 4x double");
    static_assert(sizeof(FPlane) == 32, "FPlane must be 4x double");
    static_assert(sizeof(FTransform) == 0x60, "FTransform must equal BoneStride (0x60 incl. padding)");

    struct FCameraCacheEntry
    {        float                                              Timestamp;
        unsigned char                                      UnknownData00_6[0xC];
        FMinimalViewInfo                                   POV;
    };

    


    struct FName {
        int32_t ComparisonIndex;
        int32_t Number;

        FName() : ComparisonIndex(0), Number(0) {}
        FName(int32_t index, int32_t number = 0) : ComparisonIndex(index), Number(number) {}

        inline bool operator==(const FName& other) const
        {
            return ComparisonIndex == other.ComparisonIndex;
        };
    };

    struct FNameEntryHandle
    {
        uint16_t bIsWide : 1;
        uint16_t Len : 15;
    };

    struct FNameEntry
    {
        int32_t ComparisonId;
        FNameEntryHandle Header;
        union
        {
            char        AnsiName[1024];
            wchar_t     WideName[1024];
        };
        wchar_t const* GetWideName() const { return WideName; }
        char const* GetAnsiName() const { return AnsiName; }
        bool IsWide() const { return Header.bIsWide; }

    };

    


    struct FString {
        wchar_t* Data;
        int32_t ArrayNum;
        int32_t ArrayMax;

        FString() : Data(nullptr), ArrayNum(0), ArrayMax(0) {}
    };

    


    template<typename T>
    struct TArray {
        T* Data;
        int32_t Count;
        int32_t Max;

        TArray() : Data(nullptr), Count(0), Max(0) {}

        template<typename U = T, typename std::enable_if<std::is_constructible<U, uintptr_t>::value, int>::type = 0>
        std::optional<U> TryAt(int32_t index) const {
            if (!Data || index < 0 || index >= Count) return std::nullopt;
            auto element_ptr = maxi::driver::g_driver->try_read<uintptr_t>(reinterpret_cast<uintptr_t>(Data) + static_cast<uintptr_t>(index) * sizeof(uintptr_t));
            if (!element_ptr) return std::nullopt;
            return U(*element_ptr);
        }

        template<typename U = T, typename std::enable_if<std::is_constructible<U, uintptr_t>::value, int>::type = 0>
        [[deprecated("Masks read failure as default object. Use TryAt() instead.")]]
        U operator[](int32_t index) const {
            return TryAt<U>(index).value_or(U{});
        }

        template<typename U = T, typename std::enable_if<!std::is_constructible<U, uintptr_t>::value, int>::type = 0>
        std::optional<U> TryAt(int32_t index) const {
            if (!Data || index < 0 || index >= Count) return std::nullopt;
            return maxi::driver::g_driver->try_read<U>(reinterpret_cast<uintptr_t>(Data) + static_cast<uintptr_t>(index) * sizeof(U));
        }

        template<typename U = T, typename std::enable_if<!std::is_constructible<U, uintptr_t>::value, int>::type = 0>
        [[deprecated("Masks read failure as default object. Use TryAt() instead.")]]
        U operator[](int32_t index) const {
            return TryAt<U>(index).value_or(U{});
        }


        template<typename U = T, typename std::enable_if<!std::is_constructible<U, uintptr_t>::value, int>::type = 0>
        std::optional<std::vector<U>> TryReadAllBatch() const {
            if (!Data || Count <= 0) return std::nullopt;
            return maxi::driver::g_driver->try_read_array_batch<U>(reinterpret_cast<uintptr_t>(Data), Count);
        }

        template<typename U = T, typename std::enable_if<!std::is_constructible<U, uintptr_t>::value, int>::type = 0>
        [[deprecated("Masks read failure as empty vector. Use TryReadAllBatch() instead.")]]
        std::vector<U> ReadAllBatch() const {
            return TryReadAllBatch<U>().value_or(std::vector<U>{});
        }


        template<typename U = T, typename std::enable_if<!std::is_constructible<U, uintptr_t>::value, int>::type = 0>
        std::optional<std::vector<U>> TryReadRangeBatch(int32_t start_index, int32_t count) const {
            if (!Data || start_index < 0 || start_index >= Count || count <= 0) return std::nullopt;

            int32_t actual_count = (std::min)(count, Count - start_index);
            uintptr_t start_ptr = reinterpret_cast<uintptr_t>(Data) + static_cast<uintptr_t>(start_index) * sizeof(U);

            return maxi::driver::g_driver->try_read_array_batch<U>(start_ptr, actual_count);
        }

        template<typename U = T, typename std::enable_if<!std::is_constructible<U, uintptr_t>::value, int>::type = 0>
        [[deprecated("Masks read failure as empty vector. Use TryReadRangeBatch() instead.")]]
        std::vector<U> ReadRangeBatch(int32_t start_index, int32_t count) const {
            return TryReadRangeBatch<U>(start_index, count).value_or(std::vector<U>{});
        }

        int32_t Num() const { return Count; }
        bool IsValidIndex(int32_t index) const { return index >= 0 && index < Count; }
    };

    


    template<typename KeyType, typename ValueType>
    struct TMap {
        struct TPair {
            KeyType Key;
            ValueType Value;
        };

        TPair* Data;
        int32_t Count;
        int32_t Max;

        TMap() : Data(nullptr), Count(0), Max(0) {}
    };

    


    template<typename T>
    struct TSet {
        T* Data;
        int32_t Count;
        int32_t Max;

        TSet() : Data(nullptr), Count(0), Max(0) {}
    };

    


    template<typename T>
    struct TWeakObjectPtr {
        int32_t ObjectIndex;
        int32_t ObjectSerialNumber;

        TWeakObjectPtr() : ObjectIndex(0), ObjectSerialNumber(0) {}
    };

    


    template<typename T>
    struct TSharedPtr {
        T* Object;
        void* SharedReferenceCount;

        TSharedPtr() : Object(nullptr), SharedReferenceCount(nullptr) {}
    };

    


    template<typename T>
    struct TUniquePtr {
        T* Object;

        TUniquePtr() : Object(nullptr) {}
    };

    


    template<typename... Args>
    struct FDelegate {
        void* Object;
        void* Function;

        FDelegate() : Object(nullptr), Function(nullptr) {}
    };

    


    template<typename... Args>
    struct FMulticastDelegate {
        TArray<FDelegate<Args...>> InvocationList;

        FMulticastDelegate() : InvocationList() {}
    };


    namespace Constants {
        constexpr double PI = 3.14159265358979323846264338327950288419716939937510;
        constexpr double HALF_PI = PI * 0.5;
        constexpr double TWO_PI = PI * 2.0;
        constexpr double DEG_TO_RAD = PI / 180.0;
        constexpr double RAD_TO_DEG = 180.0 / PI;
        constexpr double HALF_DEG_TO_RAD = PI / 360.0;

        constexpr double SMALL_NUMBER = 1.e-8;
        constexpr double KINDA_SMALL_NUMBER = 1.e-4;
        constexpr double BIG_NUMBER = 1.7976931348623157e+308;

        constexpr int32_t INDEX_NONE = -1;
        constexpr uint32_t MAX_uint32 = 0xFFFFFFFF;
        constexpr int32_t MAX_int32 = 0x7FFFFFFF;
        constexpr int32_t MIN_int32 = 0x80000000;
    }


    namespace Math {
        template<typename T>
        T Clamp(T value, T min, T max) {
            return (value < min) ? min : (value > max) ? max : value;
        }

        template<typename T>
        T Lerp(T a, T b, double alpha) {
            return a + (b - a) * alpha;
        }

        inline D3DMatrix toMatrix(FRotator Rotation, FVector origin)
        {
            double Pitch = ((double)Rotation.Pitch * double(Constants::DEG_TO_RAD));
            double Yaw = ((double)Rotation.Yaw * double(Constants::DEG_TO_RAD));
            double Roll = ((double)Rotation.Roll * double(Constants::DEG_TO_RAD));

            double SP = ::sin(Pitch);
            double CP = ::cos(Pitch);
            double SY = ::sin(Yaw);
            double CY = ::cos(Yaw);
            double SR = ::sin(Roll);
            double CR = ::cos(Roll);

            D3DMatrix Matrix;
            Matrix._11 = CP * CY;
            Matrix._12 = CP * SY;
            Matrix._13 = SP;
            Matrix._14 = 0.;

            Matrix._21 = SR * SP * CY - CR * SY;
            Matrix._22 = SR * SP * SY + CR * CY;
            Matrix._23 = -SR * CP;
            Matrix._24 = 0.;

            Matrix._31 = -(CR * SP * CY + SR * SY);
            Matrix._32 = CY * SR - CR * SP * SY;
            Matrix._33 = CR * CP;
            Matrix._34 = 0.;

            Matrix._41 = origin.X;
            Matrix._42 = origin.Y;
            Matrix._43 = origin.Z;
            Matrix._44 = 1.;

            return Matrix;
        }

        double FInterpTo(double current, double target, double deltaTime, double interpSpeed);
        FVector VInterpTo(const FVector& current, const FVector& target, double deltaTime, double interpSpeed);
        FRotator RInterpTo(const FRotator& current, const FRotator& target, double deltaTime, double interpSpeed);

        bool IsNearlyEqual(double a, double b, double tolerance = Constants::KINDA_SMALL_NUMBER);
        bool IsNearlyZero(double value, double tolerance = Constants::KINDA_SMALL_NUMBER);

        double FMod(double x, double y);
        double FTrunc(double value);
        double FFloor(double value);
        double FCeil(double value);
        double FRound(double value);

        FVector GetForwardVector(const FRotator& rotation);
        FVector GetRightVector(const FRotator& rotation);
        FVector GetUpVector(const FRotator& rotation);

        FRotator MakeRotFromX(const FVector& forward);
        FRotator MakeRotFromY(const FVector& right);
        FRotator MakeRotFromZ(const FVector& up);

        double Dist(const FVector& a, const FVector& b);
        double DistSquared(const FVector& a, const FVector& b);
        double Dist2D(const FVector& a, const FVector& b);
        double DistSquared2D(const FVector& a, const FVector& b);
    }

}
