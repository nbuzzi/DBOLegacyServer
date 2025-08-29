CNtlTSTrigger
{
	sm = 1;
	sq = 1;
	qc = 24;
	rq = 0;
	tid = 8001;
	title = 800102;

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
			ndesc0 = 800120;
			uspt = -1;
			desc = 800124;
			nid0 = 2;
			type = 0;
			ust = 0;
			idx = 1381112;
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
				npcidx = "1381112;";
			}
		}
	}
}