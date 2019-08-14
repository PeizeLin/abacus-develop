#ifndef CAL_R_OVERLAP_R_H
#define CAL_R_OVERLAP_R_H

#include<vector>
using std::vector;
#include<map>
using std::map;
#include<set>
using std::set;

#include "../src_lcao/center2_orb.h"
#include "../src_lcao/center2_orb-orb11.h"
#include "../src_lcao/center2_orb-orb21.h"

#include "../src_lcao/make_overlap_table.h"
#include "../src_lcao/make_gaunt_table.h"
#include "../src_lcao/numerical_orbital_lm.h"
#include "../src_lcao/lcao_orbitals.h"
#include "../src_global/vector3.h"
#include "../src_lcao/ylm.h"
#include "../src_lcao/global_fp.h"

#include "../src_pw/global.h"

class cal_r_overlap_R
{
public:
	Make_Overlap_Table MOT;
	Make_Gaunt_Table MGT;
	Numerical_Orbital_Lm orb_r;
	
	int R_x_num;
    int R_y_num;
    int R_z_num;
	int R_minX;
	int R_minY;
	int R_minZ;
	
	Vector3<double> ****psi_r_psi;
	bool allocate_psi_r_psi = false;
	
	map<size_t,
		map<size_t,
			map<size_t,
				map<size_t,
					map<size_t,
						map<size_t,
						Center2_Orb::Orb21>>>>>> center2_orb21_r;
						
						
						
	cal_r_overlap_R();
	~cal_r_overlap_R();
	
	void init();
	void out_r_overlap_R(const int nspin);
	
	int iw2it(int iw);
	int iw2ia(int iw);
	int iw2iL(int iw);
	int iw2iN(int iw);
	int iw2im(int iw);					
	
};
#endif

