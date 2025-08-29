CNtlTSTrigger
{
	sm = 1;
	sq = 0;
	qc = -1;
	rq = 0;
	tid = 8001;
	title = 800102;

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
				dx = "-0.055";
				px = "-355.206";
				widx = 960000;
				dy = "0.000000";
				py = "167.158";
				taid = 1;
				type = 1;
				dz = "-0.998";
				pz = "66.816";
			}
		}
		CDboTSContStart
		{
			cid = 0;
			stdiag = 800107;
			nolnk = 253;
			rm = 0;
			yeslnk = 1;

			CDboTSCheckLvl
			{
				maxlvl = 100;
				minlvl = 30;
			}
			CDboTSClickNPC
			{
				npcidx = "1381112;";
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
			cont = 800108;
			gtype = 1;
			oklnk = 2;
			area = 800101;
			goal = 800104;
			sort = 800105;
			prelnk = "0;";
			ds = 1;
			grade = 0;
			rwd = 100;
			title = 800102;
		}
		CDboTSContGAct
		{
			cid = 2;
			elnk = 253;
			nextlnk = 3;
			prelnk = "1;";

			CDboTSActNPCConv
			{
				conv = 800109;
				ctype = 1;
				idx = 1381112;
				taid = 2;
			}
			CDboTSActRegQInfo
			{
				cont = 800108;
				gtype = 1;
				area = 800101;
				goal = 800104;
				grade = -1;
				rwd = 100;
				scitem = -1;
				sort = 800105;
				stype = 2;
				taid = 1;
				title = 800102;
			}
		}
		CDboTSContReward
		{
			canclnk = 255;
			cid = 100;
			rwdzeny = 0;
			desc = 800114;
			nextlnk = 254;
			rwdexp = 0;
			rwdtbl = 800101;
			rwdtype = 0;
			ltime = -1;
			prelnk = "4;";
			usetbl = 1;

			CDboTSClickNPC
			{
				npcidx = "1381111;";
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
				npcidx = "1381112;";
			}
		}
	}
}

