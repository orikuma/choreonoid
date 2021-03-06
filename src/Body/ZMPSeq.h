/**
   @file
   @author Shin'ichiro NAKAOKA
*/

#ifndef CNOID_BODY_ZMP_SEQ_H_INCLUDED
#define CNOID_BODY_ZMP_SEQ_H_INCLUDED

#include <cnoid/Vector3Seq>
#include "exportdecl.h"

namespace cnoid {

class CNOID_EXPORT ZMPSeq : public Vector3Seq
{
public:
    static const std::string& key();

    ZMPSeq(int nFrames = 0);
    ZMPSeq(const ZMPSeq& org);
    ZMPSeq(const Vector3Seq& org);

    ZMPSeq& operator=(const ZMPSeq& rhs);

    virtual AbstractSeq& operator=(const AbstractSeq& rhs);
    virtual AbstractSeqPtr cloneSeq() const;

    bool isRootRelative() const { return isRootRelative_; }
    void setRootRelative(bool on) { isRootRelative_ = on; }

private:
    bool isRootRelative_;
};

typedef boost::shared_ptr<ZMPSeq> ZMPSeqPtr;
        
class BodyMotion;

CNOID_EXPORT ZMPSeqPtr getZMPSeq(const BodyMotion& motion);
CNOID_EXPORT ZMPSeqPtr getOrCreateZMPSeq(BodyMotion& motion);
CNOID_EXPORT void clearZMPSeq(BodyMotion& motion);
CNOID_EXPORT bool makeRootRelative(ZMPSeq& zmpseq, BodyMotion& motion, bool on);
}

#endif
