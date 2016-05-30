#pragma once

namespace DE {
	namespace Utils {
		namespace PlantSimulation { // TODO iterative solver
			class PlantNode;
			class BranchNode {
				PlantNode *Node1 = nullptr, *Node2 = nullptr;
				double Mass = 0.0, RelativeTargetAngle = 0.0, AngularSpeed = 0.0, Friction = 0.0, Restitution = 0.0;
				BranchNode *RelativeTarget = nullptr;
			};
			class PlantNode {
				public:

			};
		}
	}
}