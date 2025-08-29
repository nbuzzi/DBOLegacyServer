CNtlTSTrigger
{
	sm = 1;
	sq = 0;
	qc = -1;
	rq = 0;
	tid = 8000;
	title = 800002;

	CNtlTSGroup
	{
		gid = 0;

		CDboTSContGAct
		{
			cid = 4;
			elnk = 255;
			nextlnk = 100;
			prelnk = "3;";

			CDboTSActPortal
			{
				dx = "-0.138000";
				px = "3694.406006";
				widx = 1;
				dy = "0.000000";
				py = "0.000000";
				taid = 2;
				type = 1;
				dz = "-0.990000";
				pz = "2971.881104";
			}
		}
		CDboTSContStart
		{
			cid = 0;
			stdiag = 978807;
			nolnk = 253;
			rm = 0;
			yeslnk = 1;

			CDboTSCheckLvl
			{
				maxlvl = 100;
				minlvl = 1;
			}
			CDboTSClickNPC
			{
				npcidx = "5523103;";
			}
			CDboTSCheckClrQst
			{
				flink = 0;
				flinknextqid = "3701;";
				not = 0;
			}
		}
		CDboTSContEnd
		{
			cid = 254;
			prelnk = "100;";
			type = 1;
		}
		CDboTSContProposal
		{
			cancellnk = 253;
			cid = 1;
			cont = 378808;
			gtype = 1;
			oklnk = 2;
			area = 978801;
			goal = 978804;
			sort = 378805;
			prelnk = "0;";
			ds = 1;
			grade = 0;
			rwd = 100;
			title = 978802;
		}
		CDboTSContGAct
		{
			cid = 2;
			elnk = 253;
			nextlnk = 3;
			prelnk = "1;";

			CDboTSActNPCConv
			{
				conv = 378809;
				ctype = 1;
				idx = 5523103;
				taid = 2;
			}
			CDboTSActRegQInfo
			{
				cont = 378808;
				gtype = 1;
				area = 978801;
				goal = 978804;
				grade = -1;
				rwd = 100;
				scitem = -1;
				sort = 378805;
				stype = 2;
				taid = 1;
				title = 978802;
			}
		}
		CDboTSContReward
		{
			canclnk = 255;
			cid = 100;
			rwdzeny = 0;
			desc = 978814;
			nextlnk = 254;
			rwdexp = 0;
			rwdtbl = 978801;
			rwdtype = 0;
			ltime = -1;
			prelnk = "4;";
			usetbl = 1;

			CDboTSClickNPC
			{
				npcidx = "5523102;";
			}
		}
		CDboTSContEnd
		{
			cid = 253;
			prelnk = "0;1;2;";
			type = 0;
		}
		CDboTSContGCond
		{
			cid = 3;
			prelnk = "2;";
			nolnk = 255;
			rm = 0;
			yeslnk = 4;

			CDboTSClickNPC
			{
				npcidx = "5523103;";
			}
		}
	}
}

