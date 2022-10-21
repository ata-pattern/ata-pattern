#ifndef COST_QAOA2_HPP
#define COST_QAOA2_HPP

#include "CostFunc.hpp"
#include "Node.hpp"
#include <cassert>
#include <queue>

class CostQAOA2 : public CostFunc {
  public:
	int _getCost(Node * node) {
		//bool debug = node->cost > 0;//called getCost for second time on this node
		
		//temporarily increase node's cycle to account for the fact that we've already scheduled current cycle; i.e. we have to wait until next cycle until we can schedule more
		if(node->numUnscheduledGates > 0) {
			node->cycle++;
		}
		
		int cost = 0;
		int costT = 99999;
		Environment * env = node->env;
		int idealCycles[env->numPhysicalQubits];//ignoring swaps, number of busy cycles until each qubit is done forever
		for(int x = 0; x < env->numPhysicalQubits; x++) {
			idealCycles[x] = node->busyCycles(x);
			//if(idealCycles[x] == 1) idealCycles[x]--;
			if(idealCycles[x] > cost) {
				cost = idealCycles[x];
			}
		}
		
		int clatency = -1;
		
		auto iter = node->readyGates.begin();
		while(iter != node->readyGates.end()) {
			GateNode * g = *iter;
			clatency = g->optimisticLatency;
			if(g->control >= 0) {
				int physicalControl = node->laq[g->control];
				idealCycles[physicalControl] += g->optimisticLatency;
			}
			if(g->target >= 0) {
				int physicalTarget = node->laq[g->target];
				idealCycles[physicalTarget] += g->optimisticLatency;
			}
			iter++;
		}
		
		int maxminswaps = 0;
		
		iter = node->readyGates.begin();
		while(iter != node->readyGates.end()) {
			GateNode * g = *iter;
			
			int c1 = 0;
			int c2 = 0;
			if(g->control >= 0) {
				int physicalControl = node->laq[g->control];
				c1 = idealCycles[physicalControl];
			}
			if(g->target >= 0) {
				int physicalTarget = node->laq[g->target];
				c2 = idealCycles[physicalTarget];
			}
			
			//calculate min swaps needed
			if(g->target >= 0 && g->control >= 0) {
				int physicalControl = node->laq[g->control];
				int physicalTarget = node->laq[g->target];
				int minSwaps = env->couplingDistances[physicalControl*env->numPhysicalQubits + physicalTarget] - 1;
				if(minSwaps < costT) costT = minSwaps;
				int totalSwapCost = env->swapCost * minSwaps;
				
				if(minSwaps > maxminswaps) maxminswaps = minSwaps;
				
				if(c1 < c2) {
					std::swap(c1, c2);
				}
				
				int slack = c1-c2;
				int effectiveSlack = (slack/env->swapCost) * env->swapCost;
				if(effectiveSlack > totalSwapCost) {
					effectiveSlack = totalSwapCost;
				}
				
				int mutualSwapCost = totalSwapCost - effectiveSlack;
				int extraSwapCost = (0x1 & (mutualSwapCost/env->swapCost)) * env->swapCost;
				mutualSwapCost -= extraSwapCost;
				assert((mutualSwapCost % env->swapCost) == 0);
				mutualSwapCost = mutualSwapCost >> 1;
				
				int cost1 = g->optimisticLatency + g->criticality + c1 + mutualSwapCost;
				int cost2 = g->optimisticLatency + g->criticality + c2 + mutualSwapCost + effectiveSlack;
				
				if(cost1 < cost2) {
					cost1 += extraSwapCost;
				} else {
					cost2 += extraSwapCost;
				}
				
				if(cost1 > cost) {
					cost = cost1;
				}
				if(cost2 > cost) {
					cost = cost2;
				}
			}
			
			iter++;
		}
		
		int minmultiswap = 0;
		bool involved[env->numPhysicalQubits];
		for(int x = 0; x < env->numPhysicalQubits; x++) {
			involved[x] = false;
		}
		iter = node->readyGates.begin();
		while(iter != node->readyGates.end()) {
			GateNode * g = *iter;
			
			assert(g->target >= 0 && g->control >= 0);
			
			int q1 = node->laq[g->control];
			int q2 = node->laq[g->target];
			
			int dist = env->couplingDistances[q1*env->numPhysicalQubits + q2];
			
			std::queue<int> pending;
			int count = 0;
			if(!involved[q1]) {
				pending.push(q1);
				involved[q1] = true;
				count = dist - 1;
			}
			while(pending.size() > 0) {
				int q = pending.front();
				pending.pop();
				for(int x = 0; x < env->numPhysicalQubits; x++) {
					if(x == q) continue;
					
					if(env->couplingDistances[q*env->numPhysicalQubits + x] != 1) {
						continue;
					}
					
					if(involved[x]) {
						if(env->couplingDistances[q*env->numPhysicalQubits + q1] <= count) {
							count = env->couplingDistances[q*env->numPhysicalQubits + q1] - 1;
						}
						continue;
					}
					
					if(env->couplingDistances[x*env->numPhysicalQubits + q2] >= env->couplingDistances[q*env->numPhysicalQubits + q2]) {
						continue;
					}
					
					involved[x] = true;
					pending.push(x);
				}
			}
			std::swap(q1, q2);
			int count2 = 0;
			if(!involved[q1]) {
				pending.push(q1);
				involved[q1] = true;
				count2 = dist - 1;
			}
			while(pending.size() > 0) {
				int q = pending.front();
				pending.pop();
				for(int x = 0; x < env->numPhysicalQubits; x++) {
					if(x == q) continue;
					
					if(env->couplingDistances[q*env->numPhysicalQubits + x] != 1) {
						continue;
					}
					
					if(involved[x]) {
						if(env->couplingDistances[q*env->numPhysicalQubits + q1] <= count2) {
							count2 = env->couplingDistances[q*env->numPhysicalQubits + q1] - 1;
						}
						continue;
					}
					
					if(env->couplingDistances[x*env->numPhysicalQubits + q2] >= env->couplingDistances[q*env->numPhysicalQubits + q2]) {
						continue;
					}
					
					involved[x] = true;
					pending.push(x);
				}
			}
			
			int minaddlswaps = count + count2;
			minmultiswap += minaddlswaps;
			
			iter++;
		}
		
		int costV2 = (minmultiswap * env->swapCost) + (node->numUnscheduledGates * clatency);
		int maxGatesPerCycle = env->numPhysicalQubits / 2;
		costV2 = (costV2 + maxGatesPerCycle - 1) / maxGatesPerCycle;
		costV2 += (node->numUnscheduledGates > 0 ? 1 : 0);//to account for fact that we won't schedule more gates in current cycle
		int maxbusy = 0;
		for(int x = 0; x < env->numPhysicalQubits; x++) {
			int busy = node->busyCycles(x);
			if(busy > 0) busy--;
			if(busy > maxbusy) maxbusy = busy;
		}
		costV2 += maxbusy;
		//if(costV2 > cost) std::cerr << "found room for improvement " << costV2 << " vs " << cost << "\n";//todo delete
		if(costV2 > cost) cost = costV2;
		
		//Undo the change we made to node's cycle number:
		if(node->numUnscheduledGates > 0) {
			node->cycle--;
		}
		
		//add old cycles to cost
		cost += node->cycle;
		
		if(costT == 99999) costT = 0;
		node->cost2 = costT;
		
		return cost;
	}
};

#endif
