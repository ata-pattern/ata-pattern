#ifndef COST_QAOA_HPP
#define COST_QAOA_HPP

#include "CostFunc.hpp"
#include "Node.hpp"
#include <cassert>

/**
 * A cost function for the case where every unscheduled gate is in the ready list
 */
class CostQAOA : public CostFunc {
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
		
		auto iter = node->readyGates.begin();
		while(iter != node->readyGates.end()) {
			GateNode * g = *iter;
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
