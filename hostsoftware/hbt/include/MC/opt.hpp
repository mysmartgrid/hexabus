#ifndef OPT__HPP_358C1805660DFD07
#define OPT__HPP_358C1805660DFD07

namespace hbt {
namespace mc {

class Builder;

void threadTrivialJumps(Builder& b);
void pruneUnreachableInstructions(Builder& b);
void moveJumpTargets(Builder& b);

void collapseTrivialJumps(Builder& b);

}
}

#endif
