CNtlTSTrigger
{
	sm = 1;
	sq = 1;
	qc = -1;
	rq = 0;
	tid = 8003;
	title = 800302;

	CNtlTSGroup
	{
		gid = 0;

		CDboTSContStart
		{
			cid = 0;
			stdiag = 800307;
			nolnk = 253;
			rm = 0;
			yeslnk = 1;

			CDboTSCheckLvl
			{
				maxlvl = 100;
				minlvl = 29;
			}
			CDboTSClickNPC
			{
				npcidx = "1381113;";
			}
		}
		CDboTSContEnd
		{
			cid = 254;
			prelnk = "100;";
			type = 1;
		}
		CDboTSContGCond
		{
			cid = 3;
			prelnk = "2;";
			nolnk = 255;
			rm = 0;
			yeslnk = 100;
		}
		CDboTSContEnd
		{
			cid = 253;
			prelnk = "2;1;0;";
			type = 0;
		}
		CDboTSContGAct
		{
			cid = 2;
			elnk = 253;
			nextlnk = 3;
			prelnk = "1;";

			CDboTSActNPCConv
			{
				conv = 800309;
				ctype = 1;
				idx = 1381113;
				taid = 2;
			}
			CDboTSActRegQInfo
			{
				cont = 800308;
				gtype = 1;
				area = 800301;
				goal = 800304;
				grade = -1;
				rwd = 100;
				scitem = -1;
				sort = 800305;
				stype = 64;
				taid = 1;
				title = 800302;
			}
			CDboTSActItem
			{
				iidx0 = 11170533;
				stype0 = 1;
				taid = 3;
				type = 0;
			}
		}
		CDboTSContReward
		{
			canclnk = 255;
			cid = 100;
			rwdzeny = 0;
			desc = 800314;
			nextlnk = 254;
			rwdexp = 0;
			rwdtbl = 800301;
			rwdtype = 0;
			ltime = -1;
			prelnk = "3;";
			usetbl = 1;

			CDboTSClickNPC
			{
				npcidx = "1381113;";
			}
		}
		CDboTSContProposal
		{
			cancellnk = 253;
			cid = 1;
			cont = 800308;
			gtype = 1;
			oklnk = 2;
			area = 800301;
			goal = 800304;
			sort = 800305;
			prelnk = "0;";
			ds = 1;
			grade = 0;
			rwd = 100;
			title = 800302;
		}
	}
	CNtlTSGroup
	{
		gid = 254;

		CDboTSContEnd
		{
			cid = 254;
			prelnk = "1;";
			type = 0;
		}
		CDboTSContGAct
		{
			cid = 1;
			elnk = 255;
			nextlnk = 254;
			prelnk = "0;";
		}
		CDboTSContStart
		{
			cid = 0;
			stdiag = 800307;
			nolnk = 255;
			rm = 0;
			yeslnk = 1;
		}
	}
}

