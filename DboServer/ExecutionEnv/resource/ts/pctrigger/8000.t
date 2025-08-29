CNtlTSTrigger
{
	sm = 1;
	sq = 1;
	qc = 24;
	rq = 0;
	tid = 8000;
	title = 800002;

	CNtlTSGroup
	{
		gid = 0;

		CDboTSContGAct
		{
			cid = 2;
			elnk = 255;
			nextlnk = 254;
			prelnk = "1;";

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
		CDboTSContEnd
		{
			cid = 254;
			prelnk = "2;";
			type = 0;
		}
		CDboTSContUsrSel
		{
			cancellnk = 255;
			cid = 1;
			lilnk = 255;
			ndesc0 = 900;
			uspt = -1;
			desc = 99;
			nid0 = 2;
			type = 0;
			ust = 0;
			idx = 5523103;
			prelnk = "0;";
		}
		CDboTSContStart
		{
			cid = 0;
			stdiag = 978807;
			nolnk = 255;
			rm = 0;
			yeslnk = 1;

			CDboTSClickNPC
			{
				npcidx = "5523103;";
			}
		}
	}
}

